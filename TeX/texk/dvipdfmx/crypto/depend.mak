$(objdir)/md5_dgst.obj: \
	md5_locl.h \
	openssl/e_os2.h \
	./openssl/opensslconf.h \
	openssl/md5.h \
	md32_common.h \
	openssl/opensslv.h

$(objdir)/rc4_skey.obj: \
	openssl/rc4.h \
	./openssl/opensslconf.h \
	rc4_locl.h \
	openssl/opensslv.h

$(objdir)/rc4_enc.obj: \
	openssl/rc4.h \
	./openssl/opensslconf.h \
	rc4_locl.h

$(objdir)/md5_one.obj: \
	openssl/md5.h \
	openssl/e_os2.h \
	./openssl/opensslconf.h \
	openssl/crypto.h \
	./openssl/stack.h \
	./openssl/safestack.h \
	openssl/opensslv.h \
	./openssl/symhacks.h

