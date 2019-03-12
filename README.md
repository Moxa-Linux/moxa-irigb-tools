# moxa-irigb-tools

Utility for controlling DA-IRIG-B expansion module
Compile and install the IRIG-B time sync daemon

1. Make the IRIG-B time sync daemon
```
root@Moxa:/home/moxa/moxa-irigb-tools# make
make -C source/mxIrigb
make[1]: Entering directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb'
for i in mxirig unitest mxIrigUtil mxSyncTimeSvc; do \
        make -C $i; \
done
make[2]: Entering directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb/mxirig'
# For x86_64 machine, we assume the host is x86_64 machine to build the library
g++  -c mxirig.cpp
ar crv libmxirig-`uname -m`.a mxirig.o
a - mxirig.o
# For i686 machine, we use -mi686 CXXFLAGS to build the library
g++  -m32 -c mxirig.cpp -o mxirigi686.o
ar crv libmxirig-i686.a mxirigi686.o
a - mxirigi686.o
make[2]: Leaving directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb/mxirig'
make[2]: Entering directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb'
make[2]: *** unitest: No such file or directory.  Stop.
make[2]: Leaving directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb'
make[2]: Entering directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb/mxIrigUtil'
g++ -Wno-write-strings mxIrigUtil.cpp -c
g++ mxIrigUtil.o -o mxIrigUtil -L../mxirig -lmxirig-x86_64 -lrt -lm
make[2]: Leaving directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb/mxIrigUtil'
make[2]: Entering directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb/mxSyncTimeSvc'
g++  ServiceSyncTime.cpp -c
g++ ServiceSyncTime.o -o ServiceSyncTime -L../mxirig -lmxirig-x86_64 -lrt -lm
make[2]: Leaving directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb/mxSyncTimeSvc'
make[1]: Leaving directory '/workspace/da-NGS/da820c/moxa-irigb-tools/source/mxIrigb'
```

2. Install the IRIG-B time sync daemon, ServiceSyncTime.
```
root@Moxa:/home/moxa/moxa-irigb-tools# cp -a mxIrigb/mxSyncTimeSvc/ServiceSyncTime /usr/sbin/
```

3. Launch the ServiceSyncTime manually
```
root@Moxa:/home/moxa/moxa-irigb-tools# /usr/sbin/ServiceSyncTime -t 1 -s 2 -i 10
```

4. Run the ServiceSyncTime automatically at booting

For Debian system reference systemd service in package
```
root@Moxa:/home/moxa# systemctl enable mx_hsrprp.service
Created symlink /etc/systemd/system/multi-user.target.wants/mx_hsrprp.service → /lib/systemd/system/mx_hsrprp.service.
root@Moxa:/home/moxa# systemctl status mx_hsrprp.service
● mx_hsrprp.service - Moxa DA-IRIG-B daemon service
   Loaded: loaded (/lib/systemd/system/mx_hsrprp.service; enabled; vendor preset: enabled)
   Active: active (exited) since Mon 2019-03-11 15:39:05 CST; 13min ago
  Process: 309 ExecStart=/usr/sbin/mx_irigb.sh start (code=exited, status=0/SUCCESS)
 Main PID: 309 (code=exited, status=0/SUCCESS)
    Tasks: 0 (limit: 4915)
   CGroup: /system.slice/mx_hsrprp.service

Mar 11 15:39:05 Moxa systemd[1]: Starting Moxa DA-IRIG-B daemon service...
Mar 11 15:39:05 Moxa systemd[1]: Started Moxa DA-IRIG-B daemon service.

```
For Ubuntu:
```
root@Moxa:/home/moxa/moxa-irigb-tools# cp -a fakeroot/etc/init.d/mx_irigb.sh /etc/init.d/
root@Moxa:/home/moxa/moxa-irigb-tools# update-rc.d mx_irigb.sh defaults
```
For Redhat Enterprise:
```
root@Moxa:/home/moxa/moxa-irigb-tools# chkconfig --levels 2345 mx_irigb.sh on
```
