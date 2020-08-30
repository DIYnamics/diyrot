''' taskqueue
Implements a asyncio event loop + process pool which WSGI workers can interact
with via local unix sockets. The TaskQueue event loop is independent from WSGI
worker threads. Here, a main event loop handles reading/writing contents of
asyncio transports, and multiprocessing distributes CPU/IO intensive tasks. '''

import asyncio
import random
import time
from queue import SimpleQueue

# TODO: dict {id: stat} for lookups 

def worker(num: int, queue: SimpleQueue) -> None:
    while True:
        # block until arrival
        job_info = queue.get()
        try:
            # pass job_info object into processing function (which may be a C++ subprocess)
            ret = process(job_info)
        except:
            raise asyncio.CancelledError(f'Failed to process {job_info} on worker {num}')
        if ret != 0:
            # requeue if not succ
            pass

async def main():
    # Create: status hash table, circle queue, derotation queue
    _stat = {}
    _circle_queue = SimpleQueue()
    _derotation_queue = SimpleQueue()

    # Create three worker tasks to process the queue concurrently.
    tasklist = []
    for i in range(3):
        task = asyncio.create_task(worker(f'worker-{i}', queue))
        tasks.append(task)

    try:
        # await infinite tasks, respawn on fail
        await asyncio.gather(*tasks)
    except:


    print('====')
    print(f'3 workers slept in parallel for {total_slept_for:.2f} seconds')
    print(f'total expected sleep time: {total_sleep_time:.2f} seconds')


asyncio.run(main())
