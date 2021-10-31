# hs110_client
Comand line client for working with TP-Link HS110

# Build
gcc -Wall hs110_client.c -o hs110_client

# Usage
`hs110_client IP [command]`

### Examples
Get system info:\
`$ hs110_client 192.168.1.2 "{\"system\":{\"get_sysinfo\":null}}"`

Get power usage:\
`$ hs110_client 192.168.1.2 "{\"emeter\":{\"get_realtime\":null}}"`

Full list of commands: https://github.com/kettenbach-it/FHEM-TPLink-HS110/blob/master/tplink-smarthome-commands.txt
