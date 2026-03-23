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