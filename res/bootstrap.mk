bin:
	mkdir bin

bin/bootstrap: src/bootstrap.c bin
	gcc -Iinc $< -o $@

cache: bin/bootstrap
	./bin/bootstrap --cache > res/.cache

clean:
	rm -rf bin
	rm -f res/.cache