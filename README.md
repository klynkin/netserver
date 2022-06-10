# netserver
simple netserver for linux

To use it just do the following:

```
make
```

```
./netserver <portnumber>
```

Now clients can connect to the server (e.g. telnet). You should call two telnet clients with command:

```
telnet localhost <portnumber>
```

To terminate server use SIGINT command (cntrl + c)
To disconnect as a client use "shutdown" command

