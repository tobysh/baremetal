# Starting Baremetal

1. Ensure NodeJS and MongoDB are installed
    - If on arch like myself, either enable MongoDB to run on startup (in a development/daily driver environment, this adds overhead for non-dev related tasks) or start it every reboot. The commands are `sudo systemctl enable mongodb.service` and `sudo systemctl start mongodb.service` respectively.
2. Make sure .data/db exists then run `npm run db` and `npm run dev`