'''
系统中有3个协程: 生产者、计算者、消费者
生产者生产'a'、'b'、'c'、'd'、'e'、'f'、'g'、'h'八个字符
计算者从生产者取出字符，将小写字符转换为大写字符，发送给消费者
消费者从计算者取出字符，将其打印到屏幕上
此题无需使用 buffer, 使用协程切换实现
'''
from greenlet import greenlet


def producer():
    for i in range(8):
        co_caculator.switch(chr(ord('a') + i))


def consumer(char: str):
    for _ in range(8):
        print(char, end=' ')
        char = co_producer.switch()


def calculator(char: str):
    for _ in range(8):
        char = co_consumer.switch(char.upper())


co_consumer = greenlet(consumer)
co_producer = greenlet(producer)
co_caculator = greenlet(calculator)

co_producer.switch()
