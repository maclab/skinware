Skinware: The Skin Middle-ware
==============================

Skinware in its core provides a flexible data chunk transfer mechanism between
two tasks, as is suitable for a skin-related application.  Generic services
can be provided and used by starting writers and readers. Skin-tailored drivers
and users internally use writers and readers, but provide facilities over them
useful for a skin application.

All threads in the skin kernel are [URT](https://github.com/ShabbyX/URT)
real-time threads.  Skinware goes a long way with synchronization mechanisms to
ensure coherency of user data as well as consistency among users.  It also
allows users with different needs to access data on their own accord.

Skinware is done and maintained by [Maclab](http://www.maclab.dibris.unige.it/)
DIBRIS, University of Genova, Italy.  Please visit [CySkin](http://www.cyskin.com/)
for more information.

The mailing list for Skinware can be found in
[google-groups/skinware](https://groups.google.com/d/forum/skinware).
