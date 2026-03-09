#!/bin/bash
set -e

cd "$(dirname "$0")"

rm -f 311_*.csv
rm -f data/311_data.csv

mkdir -p data/nyc_311

echo "=========================================================="
echo " NYC 311 Parallel Dataset Downloader"
echo "=========================================================="
echo ""

echo {2020..2026}-{01..12} | tr ' ' '\n' | xargs -n 1 -P 12 -I {} bash -c '
  YEAR=$(echo {} | cut -d- -f1)
  MONTH=$(echo {} | cut -d- -f2)
  
  case $MONTH in
    01|03|05|07|08|10|12) END="31" ;;
    04|06|09|11)          END="30" ;;
    02)                   END="28" ;;
  esac
  
  FILE="data/nyc_311/311_${YEAR}_${MONTH}.csv"
  
  echo ">>> Downloading: ${FILE}"
  
  sleep $((RANDOM % 3))
  curl -s -S --compressed -A "Mozilla/5.0" -o "${FILE}" \
       "https://data.cityofnewyork.us/resource/erm2-nwe9.csv?\$where=created_date%20between%20'\''${YEAR}-${MONTH}-01T00:00:00.000'\''%20and%20'\''${YEAR}-${MONTH}-${END}T23:59:59.999'\''&\$limit=50000000"
       
  SIZE=$(stat -c%s "${FILE}" 2>/dev/null || echo 0)
  if [ "$SIZE" -lt 1000 ]; then
      echo "--- Empty data removed: ${FILE}"
      rm -f "${FILE}"
  else
      echo "+++ Download complete: ${FILE} (${SIZE} bytes)"
  fi
'

echo ""
echo "Done! Run benchmarks using: ./run_benchmarks.sh"
echo ""
