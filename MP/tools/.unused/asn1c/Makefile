all:
	@echo "before compile, you must install [byacc.x86_64 flex.x86_64 bison.x86_64]"
	./build.sh

clean:
	ls -l | grep ^d | awk '{ print $9 }' | xargs rm -rf
