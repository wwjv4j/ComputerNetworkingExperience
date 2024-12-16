.PHONY: GBN
GBN:
	@make -C GBN
	./check.sh -x /home/wsl/GBN/bin/stop_wait -i input.txt

.PHONY: SR
SR:
	@make -C SR
	./check.sh -x /home/wsl/SR/bin/stop_wait -i input.txt

.PHONY: TCP
TCP:
	@make -C TCPSimple
	./check.sh -x /home/wsl/TCPSimple/bin/stop_wait -i input.txt

.PHONY: RDT
RDT:
	@make -C StopWait
	./check.sh -x /home/wsl/StopWait/bin/stop_wait -i input.txt

.PHONY: clear
clear:
	@ > output.txt
	@echo "clear output.txt"