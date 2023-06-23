def produce():
    for i in range(3):
        print('produce', i)
        yield i


def consume():
    caculator = caculate()
    for i in range(3):
        item = next(caculator)
        print('       consume', item)


def caculate():
    producer = produce()
    for i in range(3):
        item = next(producer)
        print('  convert', item, "->", item * 10)
        yield item * 10


consume()
