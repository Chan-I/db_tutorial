CC := gcc
CFLAGS := -g3 -O3

db: db.c node.c

run: db
	./db mydb.db

clean:
	rm -f db *.db

test: db
	bundle exec rspec

format: *.c
	clang-format -style=Google -i *.c
