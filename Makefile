APXS = apxs
SRC  = mod_headers_whitelist.c
NAME = mod_headers_whitelist.so


all: $(NAME)
	$(APXS) -c $(SRC)

$(NAME): $(SRC)
	$(APXS) -c $(SRC)
	@if [ -f .libs/$(NAME) ]; then cp .libs/$(NAME) .; fi

install: $(NAME)
	$(APXS) -i -a $(NAME)

test:
	@echo "Checking Apache loaded modules..."
	@if command -v httpd >/dev/null 2>&1; then \
	    httpd -M | grep headers_whitelist || true; \
	elif command -v apache2 >/dev/null 2>&1; then \
	    apache2 -M | grep headers_whitelist || true; \
	else \
	    echo "Neither httpd nor apache2 found"; \
	    exit 1; \
	fi

clean:
	rm -f *.o *.lo *.slo *.la *.so
	rm -rf .libs

