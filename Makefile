PANDOC ?= pandoc

PROCESS_SRC_DIR = ../../docs/process
PROCESS_OUT_DIR = docs/process
PROCESS_TEMPLATE = templates/process.html
PROCESS_LINK_FILTER = tools/process-md-links.lua
XKB_SRC = ../../docs/xkb.md
XKB_HTML = docs/xkb.html

PROCESS_DOCS = \
	howto-compilation \
	howto-contribute \
	howto-pull-request \
	programming-language

PROCESS_HTML = $(PROCESS_DOCS:%=$(PROCESS_OUT_DIR)/%.html)

.PHONY: all process-docs xkb-doc clean

all: process-docs xkb-doc

process-docs: $(PROCESS_HTML)

xkb-doc: $(XKB_HTML)

$(PROCESS_OUT_DIR):
	mkdir -p $@

$(PROCESS_OUT_DIR)/howto-compilation.html: TITLE = How to build kbd
$(PROCESS_OUT_DIR)/howto-contribute.html: TITLE = Contributing to kbd
$(PROCESS_OUT_DIR)/howto-pull-request.html: TITLE = Pull request workflow notes
$(PROCESS_OUT_DIR)/programming-language.html: TITLE = Programming language notes

$(PROCESS_OUT_DIR)/%.html: $(PROCESS_SRC_DIR)/%.md $(PROCESS_TEMPLATE) $(PROCESS_LINK_FILTER) | $(PROCESS_OUT_DIR)
	$(PANDOC) --standalone --from=gfm --to=html5 \
		--template=$(PROCESS_TEMPLATE) \
		--lua-filter=$(PROCESS_LINK_FILTER) \
		--metadata title="$(TITLE)" \
		--metadata source_path="docs/process/$*.md" \
		--metadata site_root="../.." \
		-o $@ $<

$(XKB_HTML): $(XKB_SRC) $(PROCESS_TEMPLATE)
	$(PANDOC) --standalone --from=gfm --to=html5 \
		--template=$(PROCESS_TEMPLATE) \
		--metadata title="XKB layout conversion" \
		--metadata source_path="docs/xkb.md" \
		--metadata site_root=".." \
		-o $@ $<

clean:
	rm -f $(PROCESS_HTML) $(XKB_HTML)
