use ExtUtils::MakeMaker;

if ($ARGV[0] =~ /^LIBDIR=(.*)$/) {
   $LIBDIR = $1;
} else {
   $LIBDIR = "/var/appman";
}

sub MY::postamble {
   <<EOF

objclean :: clean
maintainer-clean :: realclean
distclean :: realclean
check :: test

all :: libtracer.\$(DLEXT)

install :: all
	\$(CP) libtracer.\$(DLEXT) \$(INSTALLSITEARCH)/libtracer.\$(DLEXT)
	\$(CHMOD) 755 \$(INSTALLSITEARCH)/libtracer.\$(DLEXT)

clean ::
	\$(RM_F) libtracer.\$(DLEXT)

libtracer.\$(DLEXT): libtracer.c
	\$(CC) \$(LDDLFLAGS) \$(CCCDLFLAGS) -O -o \$@ \$^ -lc -ldl

EOF
}

WriteMakefile(
    'dist'	=> {
	            PREOP	=> 'chmod -R u=rwX,go=rX . ;',
	            COMPRESS	=> 'gzip -9v',
	            SUFFIX	=> '.gz',
	           },
    'NAME'	=> 'App::Manager',
    'VERSION_FROM' => 'Manager.pm',
    'LIBS'	=> [''],
    'DEFINE'	=> "'-DLIBTRACER_SO=\"\$(INSTALLSITEARCH)/libtracer.\$(DLEXT)\"' '-DLIBDIR=\"$LIBDIR\"'",
    'EXE_FILES'	=> ['appman'],
    'INC'	=> '',
    'PREREQ_PM'	=> {
		    File::Spec		=> 0.7,
                    Storable		=> 0.6,
		   },
);

