PANDOC ?= pandoc

PROCESS_SRC_DIR = ../../docs/process
PROCESS_OUT_DIR = docs/process
PROCESS_TEMPLATE = templates/process.html
PROCESS_LINK_FILTER = tools/process-md-links.lua

PROCESS_DOCS = \
	howto-compilation \
	howto-contribute \
	howto-pull-request \
	programming-language

PROCESS_HTML = $(PROCESS_DOCS:%=$(PROCESS_OUT_DIR)/%.html)

.PHONY: all process-docs clean

all: process-docs

process-docs: $(PROCESS_HTML)

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
		-o $@ $<

clean:
	rm -f $(PROCESS_HTML)
