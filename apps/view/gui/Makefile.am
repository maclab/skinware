ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
             font.shf \
             room_down.bmp \
             room_east.bmp \
             room_north.bmp \
             room_south.bmp \
             room_up.bmp \
             room_west.bmp \
             logo_unige.bmp \
             logo_cyskin.bmp

install-data-local:
	-$(MKDIR_P) $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/font.shf $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/room_down.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/room_east.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/room_north.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/room_south.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/room_up.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/room_west.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/logo_unige.bmp $$HOME/.skin/gui
	-$(INSTALL_DATA) $(srcdir)/logo_cyskin.bmp $$HOME/.skin/gui
