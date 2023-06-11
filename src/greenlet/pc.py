'''
系统中有2个协程: 生产者、消费者
生产者生产'a'、'b'、'c'、'd'、'e'、'f'、'g'、'h'八个字符
消费者获得生产者传递的数据，并将其打印到屏幕上
此题无需使用 buffer, 使用协程切换实现
'''
from greenlet import greenlet


def producer():
    for i in range(8):
        co_consumer.switch(chr(ord('a') + i))


def consumer(char: str):
    for _ in range(8):
        print(char, end=' ')
        char = co_producer.switch()


co_consumer = greenlet(consumer)
co_producer = greenlet(producer)

co_producer.switch()
