#тест по блокировки всех сайтов
./tufilter --transport TCP --port 443 --filter Enable
./tufilter --transport TCP --port 80 --filter Enable
./tufilter --transport udp --port 443 --filter Enable
./tufilter --transport udp --port 80 --filter Enable
#./tufilter --transport udp --port 80 --filter disable
#./tufilter --transport udp --port 80 --filter disable
#./tufilter --show
