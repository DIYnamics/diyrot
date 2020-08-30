import asyncio
import random
import time

async def inserter(queue: asyncio.Queue) -> None:
    while True:
        for i in range(10):
            sleep_for = random.uniform(0.05, 1.0)
            await queue.put(sleep_for)
        print(f'CURRENT QUEUE SIZE: {queue.qsize()}')
        await asyncio.sleep(5)

async def worker(num: int, queue: asyncio.Queue) -> None:
    while True:
        # block until arrival
        i = await queue.get()
        print(f'worker {num} consumed {i}')
        await asyncio.sleep(i)
        #except:
        #    raise asyncio.CancelledError(f'Failed to process {i} on worker {num}')

async def main():
    queue = asyncio.Queue()
    tasks = [asyncio.create_task(inserter(queue)) for _ in range(2)] \
            + [asyncio.create_task(worker(i, queue)) for i in range(3)]
    await asyncio.gather(*tasks)

asyncio.run(main())
