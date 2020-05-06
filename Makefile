db: db.c
	gcc db.c -o db -g -O0  -gdwarf-2 -g3

run: db
	./db mydb.db

clean:
	rm -f db *.db

format: *.c
	clang-format -style=Google -i *.c
