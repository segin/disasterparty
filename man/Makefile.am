# Define the man page section
man3_MANS = \
	dp_get_version.3 \
	dp_init_context.3 \
	dp_destroy_context.3 \
	dp_perform_completion.3 \
	dp_perform_streaming_completion.3 \
	dp_free_response_content.3 \
	dp_free_messages.3 \
	dp_message_add_text_part.3 \
	dp_message_add_image_url_part.3 \
	dp_message_add_base64_image_part.3

# Source files for the man pages (using .3dp extension)
# Automake will generate .3 from .3dp if SUFFIXES and a rule are defined,
# or we can list them directly if they are pre-generated or use a simple rule.
# For simplicity, we'll assume the .3 files are generated from .3dp
# by a rule or are the same. If using a preprocessor like ronn,
# different rules would be needed.
# For groff, the source is effectively the target with a different extension.

# Tell automake about our source extension .3dp
SUFFIXES = .3dp .3

# Rule to convert .3dp to .3 (simple copy, or could be a groff command)
# This rule might be too simple if variable substitution (like @DP_VERSION@)
# from configure.ac is needed directly in the installed man page.
# A better approach is to have configure substitute variables into the .3dp files
# creating .3 files in the build directory, and then install those.
# Or, use sed during make.

# For now, let's list the sources that will be transformed.
# The .3 files will be generated in the build directory.
dist_man3_MANS = $(man3_MANS)

# If variable substitution is needed (e.g. for @DP_VERSION@ from configure.ac)
# we need to treat .3dp files as templates.
# Let's assume configure will create the final .3 files from .3dp.in or similar.
# For now, we'll assume the .3dp files are the sources and will be installed as .3
# The @DP_VERSION@ in the man pages implies they should be processed by configure.
# So, we should name them .3dp.in and have configure generate .3dp (or directly .3).

# Let's rename sources to .3.in and have configure process them.
# This requires changes to configure.ac to AC_CONFIG_FILES for each man page.
# Or, a more common approach: list source files and use a rule in Makefile.am
# to generate the .3 files.

# Simpler approach: list the source files, and they will be distributed.
# The installation will just copy them and rename.
# The @DP_VERSION@ needs to be handled. Let's assume we'll use sed in a rule.

EXTRA_DIST = \
	dp_get_version.3dp \
	dp_init_context.3dp \
	dp_destroy_context.3dp \
	dp_perform_completion.3dp \
	dp_perform_streaming_completion.3dp \
	dp_free_response_content.3dp \
	dp_free_messages.3dp \
	dp_message_add_text_part.3dp \
	dp_message_add_image_url_part.3dp \
	dp_message_add_base64_image_part.3dp

# Rule to generate .3 from .3dp, performing substitutions
# This rule will be implicitly used by Automake for targets in man3_MANS
# if their sources are .3dp
.3dp.3:
	$(AM_V_GEN)sed -e "s|@DP_VERSION@|$(DP_VERSION_FROM_CONFIGURE)|g" $< > $@

# We need to get DP_VERSION_FROM_CONFIGURE from configure.ac via AM_SUBST_NOTMAKE
# or pass it directly. Let's pass it from the top Makefile.am.
# This requires DP_VERSION_FROM_CONFIGURE to be set in the environment for make.
# A cleaner way is to have configure generate the final .3 files.

# Alternative: configure generates the .3 files directly.
# In configure.ac:
# AC_CONFIG_FILES([man/dp_get_version.3 ...])
# And then in man/Makefile.am, just list man3_MANS.
# The .3 files would be generated from .3.in files.

# Let's stick to the simpler approach for now and handle substitution via a rule.
# The .3 files are what get installed.
# The source files are .3dp.
# We need to ensure that make knows how to build a .3 from a .3dp.

# Clean up generated .3 files
CLEANFILES = $(man3_MANS)

# We need to pass the version to this Makefile.am
# This is usually done via AC_SUBST in configure.ac and then using it here.
# The @DP_VERSION@ in the .TH line of man pages needs to be substituted.
# Let's assume `configure` substitutes `DP_VERSION` which is already available.
# The rule below uses `$(VERSION)` which is standard from AC_INIT.

.3dp.3:
	@echo "Generating $@"
	$(AM_V_GEN)sed -e "s|@DP_VERSION@|$(VERSION)|g" $< > $@

# Ensure that the man pages are rebuilt if configure.ac changes (due to VERSION)
# This is complex. A common way is to make .3 files depend on configure.
# For now, this rule should work if `make` is run after `./configure`.

