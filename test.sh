#тест по блокировки всех сайтов
./tufilter --transport TCP --port 443 --filter Enable
./tufilter --transport tcp --port 80 --filter Enable
./tufilter --transport udp --port 443 --filter Enable
./tufilter --transport uDp --port 80 --filter Enable
./tufilter --transport uDp --port 81 --filter Enable --route output
./tufilter --transport uDp --port 81 --filter Enable --route input
#./tufilter --transport udp --port 80 --filter disable
#./tufilter --transport udp --ip 8.8.8.8 --port 80 --filter Enable
#./tufilter --transport udp --ip 8.8.8.8 --filter Enable
#./tufilter --show
