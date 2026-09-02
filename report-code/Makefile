# =====================================================================
# Makefile — build Báo cáo Chi tiết Code UMT-TBS (tiếng Việt, pdflatex)
#
#   make            -> build xuất UMT_TBS_CodeDetail.pdf (3 lượt pdflatex)
#   make clean      -> xoá artifact build
#   make verify     -> build + kiểm tra PDF tồn tại + grep từ khóa
# =====================================================================

LATEX    := pdflatex -interaction=nonstopmode -halt-on-error
BIBTEX   := bibtex
PDF      := UMT_TBS_CodeDetail

.PHONY: all clean verify

all:
	$(LATEX) main.tex
	$(LATEX) main.tex
	$(LATEX) main.tex
	cp main.pdf $(PDF).pdf
	@echo "==> $(PDF).pdf"

clean:
	rm -f main.aux main.toc main.out main.log main.lof main.lot \
	      main.fls main.fdb_latexmk main.synctex.gz main.pdf $(PDF).pdf

# Verify: build + báo cáo xem PDF có chứa các từ khóa chính không.
verify: all
	@if command -v pdftotext >/dev/null 2>&1; then \
	  pdftotext $(PDF).pdf - > /tmp/umt_tbs_code.txt; \
	  for kw in "Cảm biến" "Mạng" "Xử lý" "Ứng dụng" "Nối các lớp"; do \
	    grep -q "$$kw" /tmp/umt_tbs_code.txt && echo "OK keyword: $$kw" \
	      || { echo "MISSING keyword: $$kw"; exit 1; }; \
	  done; \
	else echo "pdftotext không có — bỏ qua keyword check"; fi