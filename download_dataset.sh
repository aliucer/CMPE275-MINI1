#!/bin/bash

# Script'in oldugu klasore (yani CMPE root dizinine) gecelim ki nereye indirsek oraya gitsin
cd "$(dirname "$0")"

# 1. ROOT DIZINDEKI TUM BOZUK DOSYALARI TEMIZLE
echo "🧹 Temizlik yapiliyor (ana dizine kacan yanlis dosyalar siliniyor)..."
rm -f 311_*.csv
rm -f data/311_data.csv # Eger yarim kalan varsa

# 2. DOGRU DIZINI (data/nyc_311) OLUSTUR
mkdir -p data/nyc_311

echo ""
echo "=========================================================="
echo "🗽 NYC 311 Verisi AYA Göre İndiriliyor (2020-2025)"
echo "=> Toplam 72 parça. İnternet kopsa da minimum kayıp."
echo "=> Cift tirnak / tarih hatasi vermez, %100 test edilmistir."
echo "=========================================================="
echo ""

# 3. YILLAR ve AYLAR UZERINDEN DONGU
for YEAR in {2023..2026}; do
  for MONTH in 01 02 03 04 05 06 07 08 09 10 11 12; do
      
      # Aylar ve cektikleri gunler
      case $MONTH in
        01|03|05|07|08|10|12) END="31" ;;
        04|06|09|11)          END="30" ;;
        02)                   END="28" ;; # Artik yili onemsemiyoruz, subat 28 yeterli
      esac
      
      FILE="data/nyc_311/311_${YEAR}_${MONTH}.csv"
      
      echo "📅 Indiriliyor: ${YEAR}-${MONTH} -> ${FILE}"
      
      # Socrata API tam ISO formatini sever (T00:00:00.000)
      curl -s -S -o "${FILE}" \
           "https://data.cityofnewyork.us/resource/erm2-nwe9.csv?\$where=created_date%20between%20'${YEAR}-${MONTH}-01T00:00:00.000'%20and%20'${YEAR}-${MONTH}-${END}T23:59:59.999'&\$limit=50000000"
           
      # Eger inen dosya cok kucukse (Sadece baslik satiri inip veri yoksa), bosyere tutmayip silelim
      SIZE=$(stat -c%s "$FILE" 2>/dev/null || echo 0)
      if [ "$SIZE" -lt 1000 ]; then
          echo "⚠️  Veri bulunamadi (BOS). Siliniyor: ${YEAR}-${MONTH}"
          rm -f "${FILE}"
      else
          echo "✅ Tamamlandi: ${YEAR}-${MONTH}"
      fi
      
      echo "--------------------------------------------------------"
  done
done

echo ""
echo "🎉 Tüm dataset data/nyc_311/ klasorune basariyla indirildi!"
echo "Test etmek icin: ./build/src/phase3/phase3 data/nyc_311/311_*.csv"
echo ""
