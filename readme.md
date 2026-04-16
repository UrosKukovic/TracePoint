# List all containers
docker ps -a

# Run timescaleDB in pgadmin4
docker start tracepoint-db pgadmin-dashboard

# PgAdmin4
u: uros@tracepoint.com
p: admin

# DB
Server name: TracePoint-Local
IP: 172.17.0.1
port: 5432

u: postgres
p: postgres
db name: tracepoint

# PlatformIO
pio device monitor --port /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_D8:3B:DA:49:D8:4C-if00 --baud 115200