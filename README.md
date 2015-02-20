Skinware: The Skin Middle-ware
==============================

Skinware was implemented as part of the European Union project ROBOSKIN.  This middle-ware consists of
a kernel module (aka _skin kernel_) and a user-space library.  The skin kernel provides an API for _drivers_ to be
written for different kinds of robotic skins.  Different drivers can connect to the skin kernel providing data from
sensors in different _layers_ of the skin.

The skin kernel also provides an API for _services_ that get sensor data and provide data of their own.  These
processed data can be used applications or other services.

The user-space library gets sensor data from the skin kernel and provides it to the users.

All threads in the skin kernel are [RTAI](http://www.rtai.org) real-time threads.  Skinware goes a long
way with synchronization mechanisms to ensure coherency of user data as well as consistency among users.  It also
allows users with different needs to access data on their own accord.

Skinware is done and maintained by Maclab, DIBRIS, University of Genova, Italy.
Please visit [maclab](http://www.maclab.dibris.unige.it/) for more information.

The mailing list for Skinware can be found in
[google-groups/skinware](https://groups.google.com/d/forum/skinware).
