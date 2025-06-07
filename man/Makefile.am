# man/Makefile.am

# List of man pages to install in section 3.
# These are the target .3 files that will be generated from .3dp sources.
man3_MANS = \
	dp_get_version.3 \
	dp_init_context.3 \
	dp_destroy_context.3 \
	dp_perform_completion.3 \
	dp_perform_streaming_completion.3 \
	dp_perform_anthropic_streaming_completion.3 \
	dp_free_response_content.3 \
	dp_free_messages.3 \
	dp_message_add_text_part.3 \
	dp_message_add_image_url_part.3 \
	dp_message_add_base64_image_part.3 \
	dp_list_models.3 \
	dp_free_model_list.3 \
	dp_serialize_messages_to_json_str.3 \
	dp_deserialize_messages_from_json_str.3 \
	dp_serialize_messages_to_file.3 \
	dp_deserialize_messages_from_file.3 \
	dp_serialize.3

# List of man pages to install in section 7.
man7_MANS = \
	disasterparty.7

# Source files (.3dp and .7dp templates) that need to be included in the distribution tarball.
EXTRA_DIST = $(man3_MANS:.3=.3dp) $(man7_MANS:.7=.7dp)

# Declare the suffixes used for the transformation rule.
# This needs to be done before the rule itself if it's not a default rule.
SUFFIXES = .3 .3dp .7 .7dp

# Rule to transform a .3dp file into a .3 file.
.3dp.3:
	@echo "Generating man page $@"
	$(AM_V_GEN)$(SED) \
		-e 's|@DP_VERSION@|$(VERSION)|g' \
		-e 's|@PACKAGE_BUGREPORT@|$(PACKAGE_BUGREPORT)|g' \
		$< > $@

# Rule to transform a .7dp file into a .7 file.
.7dp.7:
	@echo "Generating man page $@"
	$(AM_V_GEN)$(SED) \
		-e 's|@DP_VERSION@|$(VERSION)|g' \
		-e 's|@PACKAGE_BUGREPORT@|$(PACKAGE_BUGREPORT)|g' \
		$< > $@

# Files to be removed by 'make clean'.
CLEANFILES = $(man3_MANS) $(man7_MANS)
