#!/bin/bash
IMPLEMENTACIONES=("mutex-cond" "spin-lock" "sem" "starving")
FILOSOFOS=(5 10 15 20)
REPETICIONES=3
DURACION=20

DIR_FUENTE="Dining Philosopher"

mkdir -p output/raw output/img

if ! [ -x "$(command -v /usr/bin/time)" ]; then
  echo "Error: El comando '/usr/bin/time' no se encuentra o no es ejecutable."
  exit 1
fi

if ! [ -x "$(command -v pidstat)" ]; then
  echo "Error: El comando 'pidstat' no se encuentra o no es ejecutable."
  exit 1
fi

if ! [ -x "$(command -v pgrep)" ]; then
  echo "Error: El comando 'pgrep' no se encuentra o no es ejecutable."
  exit 1
fi

run_test() {
  local impl=$1
  local n=$2
  local rep=$3
  local ruta_bin="${DIR_FUENTE}/${impl}"
  local pidstat_log_file="output/raw/cpu_${impl}_${n}_${rep}.txt"
    
  local combined_metrics_file="output/raw/combined_metrics_${impl}_${n}_${rep}.txt"
  local time_log_file="output/raw/time_${impl}_${n}_${rep}.txt"
  local mem_log_file="output/raw/mem_peak_${impl}_${n}_${rep}.txt"
  local ctx_voluntary_log_file="output/raw/ctx_voluntary_${impl}_${n}_${rep}.txt"
  local ctx_involuntary_log_file="output/raw/ctx_involuntary_${impl}_${n}_${rep}.txt"

  echo "[+$rep] ejecutando $impl con $n filosofos"

  if [ ! -f "$ruta_bin" ]; then
    echo "Error: Binario $ruta_bin no encontrado para $impl con $n (rep $rep)."
    echo "# BINARIO NO ENCONTRADO $ruta_log_file" > "$pidstat_log_file"
    echo "0 0 0 0%" > "$time_log_file"
    echo "0" > "$mem_log_file"
    echo "0" > "$ctx_voluntary_log_file"
    echo "0" > "$ctx_involuntary_log_file"
    return 1
  fi

  (
    if [ "$DURACION" -gt 1 ]; then
      pidstat -u -h 1 $((DURACION - 1)) > "$pidstat_log_file" 2>/dev/null &
      pidstat_pid=$!
    else
      echo "# Duración de la prueba ($DURACION s) muy corta para pidstat" > "$pidstat_log_file"
    fi

    /usr/bin/time -o "$combined_metrics_file" -f "Time: %e %U %S %P\nMem: %M\nCtxVoluntary: %w\nCtxInvoluntary: %c" \
      timeout -k 5s "$DURACION" "$ruta_bin" "$n" > /dev/null 2>&1 &
    time_pid=$!

    wait "$time_pid"
    exit_status=$?
    if [ "$exit_status" -ne 0 ] && [ "$exit_status" -ne 124 ]; then
        echo "Error en la ejecución de $ruta_bin. Código de salida: $exit_status"
    fi

    if [ -n "$pidstat_pid" ] && ps -p "$pidstat_pid" > /dev/null; then
      wait "$pidstat_pid" 2>/dev/null
    fi

    if [ -f "$combined_metrics_file" ] && [ -s "$combined_metrics_file" ]; then
      grep "^Time:" "$combined_metrics_file" | awk '{print $2, $3, $4, $5}' > "$time_log_file"
      grep "^Mem:" "$combined_metrics_file" | awk '{print $2}' > "$mem_log_file"
      grep "^CtxVoluntary:" "$combined_metrics_file" | awk '{print $2}' > "$ctx_voluntary_log_file"
      grep "^CtxInvoluntary:" "$combined_metrics_file" | awk '{print $2}' > "$ctx_involuntary_log_file"
    else
      echo "0 0 0 0%" > "$time_log_file"
      echo "0" > "$mem_log_file"
      echo "0" > "$ctx_voluntary_log_file"
      echo "0" > "$ctx_involuntary_log_file"
    fi
      
    if [ -f "$pidstat_log_file" ] && [ -s "$pidstat_log_file" ]; then
      sleep 0.5 
      target_pid=$(pgrep -P "$$" -f "$ruta_bin")
      
      if [ -n "$target_pid" ]; then
        awk -v pid="$target_pid" '$3 == pid {print $0}' "$pidstat_log_file" > "$pidstat_log_file.tmp"
        if [ -s "$pidstat_log_file.tmp" ]; then
            mv "$pidstat_log_file.tmp" "$pidstat_log_file"
        else
            rm -f "$pidstat_log_file.tmp"
        fi
      fi
    fi
  ) &

  wait

  if [ ! -s "$time_log_file" ]; then echo "0 0 0 0%" > "$time_log_file"; fi
  if [ ! -s "$mem_log_file" ]; then echo "0" > "$mem_log_file"; fi
  if [ ! -s "$ctx_voluntary_log_file" ]; then echo "0" > "$ctx_voluntary_log_file"; fi
  if [ ! -s "$ctx_involuntary_log_file" ]; then echo "0" > "$ctx_involuntary_log_file"; fi
  if [ ! -s "$pidstat_log_file" ]; then echo "# No se pudieron obtener datos de pidstat" > "$pidstat_log_file"; fi
}

for impl in "${IMPLEMENTACIONES[@]}"; do
  for n in "${FILOSOFOS[@]}"; do
    for rep in $(seq 1 "$REPETICIONES"); do
      run_test "$impl" "$n" "$rep"
    done
  done
done

echo "Implementacion,Filosofos,Repeticion,TiempoReal,TiempoUsuario,TiempoSistema,UsoCPU_time,UsoCPU_pidstat_avg,PicoMemoria,CtxVoluntarios,CtxInvoluntarios" > output/summary.csv

for impl in "${IMPLEMENTACIONES[@]}"; do
  for n in "${FILOSOFOS[@]}"; do
    for rep in $(seq 1 "$REPETICIONES"); do
      time_output_file="output/raw/time_${impl}_${n}_${rep}.txt"
      mem_output_file="output/raw/mem_peak_${impl}_${n}_${rep}.txt"
      cpu_pidstat_file="output/raw/cpu_${impl}_${n}_${rep}.txt"
      ctx_voluntary_output_file="output/raw/ctx_voluntary_${impl}_${n}_${rep}.txt"
      ctx_involuntary_output_file="output/raw/ctx_involuntary_${impl}_${n}_${rep}.txt"

      real="0" user="0" sys="0" cpu_time_perc_val="0"
      mem="0"
      cpu_pidstat_avg="0.00"
      ctx_vol="0"
      ctx_invol="0"

      if [ -f "$time_output_file" ] && [ -s "$time_output_file" ]; then
        measurement_line=$(grep -Eo '^[0-9.]+[[:space:]]+[0-9.]+[[:space:]]+[0-9.]+[[:space:]]+[0-9.]+%?$' "$time_output_file" | head -n 1)
        if [ -n "$measurement_line" ]; then
          read -r real_tmp user_tmp sys_tmp cpu_time_perc_str <<< "$measurement_line"
          real=${real_tmp:-$real}
          user=${user_tmp:-$user}
          sys=${sys_tmp:-$sys}
          if [ -n "$cpu_time_perc_str" ]; then
            cpu_time_perc_val=${cpu_time_perc_str%\%}
          else
            cpu_time_perc_val="0"
          fi
        fi
      fi

      if [ -f "$mem_output_file" ] && [ -s "$mem_output_file" ]; then
        measurement_line_mem=$(grep -Eo '^[0-9]+$' "$mem_output_file" | head -n 1)
        if [ -n "$measurement_line_mem" ]; then
            mem="$measurement_line_mem"
        fi
      fi

      if [ -f "$cpu_pidstat_file" ] && [ -s "$cpu_pidstat_file" ]; then
        cpu_values=$(awk 'NR > 3 && $1 ~ /^[0-9]{2}:/ && NF >= 8 {print $8}' "$cpu_pidstat_file" 2>/dev/null)
        if [ -n "$cpu_values" ]; then
          cpu_pidstat_avg=$(echo "$cpu_values" | LC_ALL=C awk '{sum+=$1; count++} END {if (count > 0) printf "%.2f", sum/count; else print "0.00"}')
        fi
      fi
      
      if [ -f "$ctx_voluntary_output_file" ] && [ -s "$ctx_voluntary_output_file" ]; then
        ctx_vol_val=$(cat "$ctx_voluntary_output_file" | head -n 1)
        ctx_vol=${ctx_vol_val:-0}
      fi

      if [ -f "$ctx_involuntary_output_file" ] && [ -s "$ctx_involuntary_output_file" ]; then
        ctx_invol_val=$(cat "$ctx_involuntary_output_file" | head -n 1)
        ctx_invol=${ctx_invol_val:-0}
      fi
      
      echo "$impl,$n,$rep,$real,$user,$sys,$cpu_time_perc_val,$cpu_pidstat_avg,$mem,$ctx_vol,$ctx_invol" >> output/summary.csv
    done
  done
done

python3 << EOF
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import numpy as np

sns.set(style="whitegrid")

try:
    df = pd.read_csv("output/summary.csv")

    if df.empty:
        print("Error: El archivo summary.csv está vacío.")
        raise ValueError("DataFrame vacío")

    numeric_cols = ['TiempoReal', 'TiempoUsuario', 'TiempoSistema', 'UsoCPU_time', 
                    'UsoCPU_pidstat_avg', 'PicoMemoria', 
                    'CtxVoluntarios', 'CtxInvoluntarios']
    for col in numeric_cols:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    
    df = df.dropna(subset=numeric_cols)
    
    if df.empty:
        print("Error: El DataFrame está vacío después de limpiar valores no numéricos.")
        raise ValueError("DataFrame vacío después de dropna")
    
    plt.figure(figsize=(10, 7))
    sns.barplot(x='Filosofos', y='TiempoReal', hue='Implementacion', data=df)
    plt.title('Tiempo Real de Ejecución (s)')
    plt.ylabel('Segundos')
    plt.savefig('output/img/TiempoReal_Execution.png', dpi=300)
    plt.close()
    print("Info: Gráfica TiempoReal_Execution.png generada.")

    plt.figure(figsize=(10, 7))
    sns.barplot(x='Filosofos', y='UsoCPU_pidstat_avg', hue='Implementacion', data=df)
    plt.title('Uso de CPU Promedio (%)')
    plt.ylabel('Porcentaje CPU (%)')
    plt.savefig('output/img/UsoCPU_Average.png', dpi=300)
    plt.close()
    print("Info: Gráfica UsoCPU_Average.png generada.")

    plt.figure(figsize=(10, 7))
    sns.barplot(x='Filosofos', y='PicoMemoria', hue='Implementacion', data=df)
    plt.title('Pico de Memoria Máxima (KB)')
    plt.ylabel('KB')
    plt.savefig('output/img/PeakMemory.png', dpi=300)
    plt.close()
    print("Info: Gráfica PeakMemory.png generada.")

    plt.figure(figsize=(10, 7))
    sns.barplot(x='Filosofos', y='CtxVoluntarios', hue='Implementacion', data=df)
    plt.title('Cambios de Contexto Voluntarios (n)')
    plt.ylabel('Número de Cambios')
    plt.ticklabel_format(style='plain', axis='y')
    plt.savefig('output/img/CtxVoluntary.png', dpi=300)
    plt.close()
    print("Info: Gráfica CtxVoluntary.png generada.")

    plt.figure(figsize=(10, 7))
    sns.barplot(x='Filosofos', y='CtxInvoluntarios', hue='Implementacion', data=df)
    plt.title('Cambios de Contexto Involuntarios (n)')
    plt.ylabel('Número de Cambios')
    plt.ticklabel_format(style='plain', axis='y')
    plt.savefig('output/img/CtxInvoluntary.png', dpi=300)
    plt.close()
    print("Info: Gráfica CtxInvoluntary.png generada.")
    
    for impl in df['Implementacion'].unique():
        for n_filos in df['Filosofos'].unique():
            plt.figure(figsize=(10, 6))
            subset_data_found_for_plot = False
            for rep_num in range(1, int("${REPETICIONES}") + 1):
                cpu_file_path = f'output/raw/cpu_{impl}_{n_filos}_{rep_num}.txt'
                try:
                    if os.path.exists(cpu_file_path) and os.path.getsize(cpu_file_path) > 0:
                        data_cpu = pd.read_csv(cpu_file_path, delim_whitespace=True, skiprows=3, header=None, comment='#')

                        if not data_cpu.empty:
                            if data_cpu.shape[1] >= 8:
                                data_cpu.columns = ['Time', 'UID', 'PID_val', '%usr', '%system', '%guest', '%wait', 'CPU_Percentage', 'CPU_Core', 'Command_val'][:data_cpu.shape[1]]
                                column_to_plot_ts = 'CPU_Percentage'

                                data_cpu[column_to_plot_ts] = pd.to_numeric(data_cpu[column_to_plot_ts], errors='coerce')
                                data_cpu.dropna(subset=[column_to_plot_ts], inplace=True)

                                if not data_cpu.empty:
                                    plt.plot(data_cpu[column_to_plot_ts].values, label=f'Rep {rep_num}')
                                    subset_data_found_for_plot = True
                except Exception as e_file:
                    print(f"Advertencia: No se pudo procesar {cpu_file_path} para la gráfica de series temporales de CPU: {str(e_file)}")
                    continue
            
            if subset_data_found_for_plot:
                plt.title(f'Uso de CPU a lo largo del tiempo - {impl} con {n_filos} filósofos')
                plt.xlabel('Muestras de pidstat (cada 1s)')
                plt.ylabel('Uso de CPU (%)')
                plt.legend()
                plt.savefig(f'output/img/cpu_timeseries_{impl}_{n_filos}.png', dpi=300)
                plt.close()
                print(f"Info: Gráfica cpu_timeseries_{impl}_{n_filos}.png generada.")
            else:
                print(f"Info: No hay datos de CPU para graficar series temporales para {impl} con {n_filos} filósofos.")
                plt.close()

except Exception as e_main_python:
    print(f"Error principal en el script de Python: {str(e_main_python)}")

EOF

echo "[+] benchmark completado"
