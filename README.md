# Cloudflare-DNS-IP-Updater
A tool to automatically update the IP address of DNS entries in Cloudflare for those like myself without a static IP.

Uses Ipify to retrieve your public IP and Cloudflare's API to update the DNS entry.

Currently only supports one A record entry for my personal use case. If you wish to use you will have to put in your own Cloudflare API keys and zone ID as well as your A record name.

Requires cUrl for the People and Nohlmann's JSON library to compile.

Pass -r with an integer value to set the update rate in seconds, default is update every 60 seconds.
