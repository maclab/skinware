.PHONY: all clean rosskin
all: setup.bash rosskin
rosskin:
	@./make_help
clean:
	@$(MAKE) --no-print-directory -C rosskin clean

setup.bash: bootstrap
	./bootstrap
