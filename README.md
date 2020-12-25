# Guimachi

Graphical frontend for Hamachi written in Qt.

Connects directly to the service, no command output parsing: instant updates.

Not every feature is implemented or read correctly from the service, but the basic functionality works.
Missing functionality can be worked around by using the cli tool.

## Building

You need Qt >=5.11 and a C++17 compiler to build this program.

```
qmake guimachi.pro
make -j8
```

## Features

* **Instant messaging (Peer & Network)**
* Connect/Disconnect
* Connection state
* Network listing
* Peer activity
* Other useful but small options

## Missing functionality

There is a lot of missing functionality that's not implemented in a GUI, even though the engine interface itself is implemented.

The majority of complex packets sent by the service are too hard to understand due to the large amount of irrelevant/unused data they contain.

The frontend should work for typical use cases, but not every configuration will work.
