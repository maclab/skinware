import sys
import signal
# import threading is important for it calls PyEval_InitThreads()
import threading
sys.path.insert(0, 'build/lib.linux-i686-2.6')
sys.path.insert(0, '../build/lib.linux-i686-2.6')
import pyskin
import example_pyservice

def service_func(mem):
	for i in range(0, 10):
		elem = example_pyservice.at_index(mem, i)
		elem.set_x(elem.get_x() + 1)
		elem.set_y(elem.get_y() + 2)

def sighandler(signal, frame):
	sys.exit(0)

signal.signal(signal.SIGINT, sighandler)
print 'Exit with CTRL-C'
skin = pyskin.object()
skin.load()
sm = skin.service_manager()
sid = sm.initialize_sporadic_service(example_pyservice.MEMNAME,
	example_pyservice.elem_size(), 10,
	example_pyservice.REQUEST, example_pyservice.RESPONSE)
sm.start_service(sid, service_func)
signal.pause()
