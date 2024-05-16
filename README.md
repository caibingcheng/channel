### channel

**channel** is a output redirection tool on Linux, relies on the network application layer. As no authentication is required, it is not recommended to use it in a public network.

### Usage

#### Basic

Server:
```shell
$ echo "Hello, World!" | channel -s
Hello, World!
```

Client:
```shell
$ channel
Hello, World!
```

#### Embedded system

It is especially beneficial in embedded systems when resources are insufficient for log files or log processing.

On some embedded systems, the disk space is limited, we cannot store full log files when doing performance testing. **channel** can transmit log in realtime to the host, and the host can write to file or process it.

On the other hand, the embedded system is usually not equipped with powerful environment, such as python/ruby, now we can transmit the output to the host and process it.

Embedded system:
```shell
$ echo "Hello, World!" | channel -s
Hello, World!
# or transmit top command output
$ top -n 1000 -d 1 -b | channel -s
# or transmit application output
$ <your app> 2>&1 | channel -s
```

Host:
```shell
# just receive and write to file, because the host disk space is sufficient
$ channel -i <embedded system ip> | tee log.txt
# or receive and process by another command, such as python script
$ channel -i <embedded system ip> | <another command>
```

#### Share

Share realtime log with teamates, not copy files anymore.

Your machine:
```shell
$ echo "Hello, World!" | channel -s
Hello, World!
# or transmit log from embedded system
$ channel -i <embedded system ip> | channel -s
```

Your teamate's machine:
```shell
$ channel -i <your machine ip>
Hello, World!
```

### Compile

```shell
$ make
$ make install
```

Or cross compile for ARM:
```shell
$ make CROSS_COMPILE=aarch64-linux-gnu-
$ make install
```
