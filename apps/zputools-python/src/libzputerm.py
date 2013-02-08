import multiprocessing
import readline
import logging
import os

outlock = multiprocessing.Lock()

class InputThread(multiprocessing.Process):
	
	def __init__(self, zpufile):
		multiprocessing.Process.__init__(self)
		
		self.zpu = open(zpufile, 'w')
		self.stopped = False
		self.prompt = "\033[01;33mzpu-terminal\033[0m >"
		
	def run(self):
		while not self.stopped:
			with outlock:
				print self.prompt, 
				
			try:
				input = raw_input()
			except: break
			
			self.zpu.write(input)
			self.zpu.flush()
			logging.debug("Wrote %s bytes to zpu.", len(input))
		self.zpu.close()


class OutputThread(multiprocessing.Process):
	
	def __init__(self, zpufile):
		multiprocessing.Process.__init__(self)
		
		self.zpu = os.open(zpufile, os.O_RDONLY)
		self.stopped = False
		
	def run(self):
		while not self.stopped:
			try:
				output = os.read(self.zpu, 1024)
			except KeyboardInterrupt: break
			
			logging.debug("Read %d bytes from ZPU.", len(output))
			with outlock:
				print output
		os.close(self.zpu)
