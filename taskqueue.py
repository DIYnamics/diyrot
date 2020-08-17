''' taskqueue
Implements a asyncio event loop + process pool which WSGI workers can interact
with via local unix sockets. The TaskQueue event loop is independent from WSGI
worker threads. Here, a main event loop handles reading/writing contents of
asyncio transports, and a ProcessPool distributes CPU/IO intensive tasks. '''

import asyncio
import random
import time
from jobinfo import PROC_SUCC

async def worker(num: int, queue: asyncio.Queue) -> None:
    while True:
        job_info = await queue.get()
        try:
            ret = await process(job_info)
        except:
            raise asyncio.CancelledError(f'Failed to process {job_info} on worker {num}')
        if ret != PROC_SUCC:
            # requeue if not succ
        else:
            queue.task_done()

async def main():
# Create a queue that we will use to store our "workload".
    queue = asyncio.Queue()

    # Generate random timings and put them into the queue.
    total_sleep_time = 0
    for _ in range(20):
        sleep_for = random.uniform(0.05, 1.0)
        total_sleep_time += sleep_for
        queue.put_nowait(sleep_for)

    # Create three worker tasks to process the queue concurrently.
    tasks = []
    for i in range(3):
        task = asyncio.create_task(worker(f'worker-{i}', queue))
        tasks.append(task)

    # Wait until the queue is fully processed.
    started_at = time.monotonic()
    await queue.join()
    total_slept_for = time.monotonic() - started_at

    # Cancel our worker tasks.
    for task in tasks:
        task.cancel()
    # Wait until all worker tasks are cancelled.
    await asyncio.gather(*tasks, return_exceptions=True)

    print('====')
    print(f'3 workers slept in parallel for {total_slept_for:.2f} seconds')
    print(f'total expected sleep time: {total_sleep_time:.2f} seconds')


asyncio.run(main())
