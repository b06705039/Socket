Compile:
	make clean
	make
Execute:
	./server <portNum>
Handle command:
	<received msg>		<reply msg>

	REGISTER#<name>		100 OK
	<name>#<portNum>	<balance>
	List			number of account online: <number>
				<name>#<ip>#<portNum>
	Exit			Bye
Environment:
	- Mac terminal


	