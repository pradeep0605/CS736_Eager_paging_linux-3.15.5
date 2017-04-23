if [ $# -eq 1 ]; then
	if [ "$1" =  "list" ]; then
		echo "Relisting all the files"
		find arch/ -name "*.[ch]" > cscope.files
		find mm/ -name "*.[ch]" >> cscope.files
		find ipc/ -name "*.[ch]" >> cscope.files
		find kernel/ -name "*.[ch]" >> cscope.files
		find include/ -name "*.[ch]" >> cscope.files
		find fs/ -name "*.[ch]" >> cscope.files
		find lib/ -name "*.[ch]" >> cscope.files
		find net/ -name "*.[ch]" >> cscope.files
		find init/ -name "*.[ch]" >> cscope.files
	fi
fi

ctags -R `cat cscope.files`
cscope -bqk

