import gdb

f = open("log_dump.txt", "w")


class thread_log_dump(gdb.Command):
    def __init__(self):
        super(thread_log_dump, self).__init__("thread_log_dump",
                                              gdb.COMMAND_DATA)

    def invoke(self, connection, from_tty):
        conn = gdb.parse_and_eval(connection)
        nsessions = conn['session_cnt']
        for i in range(nsessions):
            log = conn['sessions'][i]['thread_log']
            f.write('Session ' + str(i))
            counter = int(log['counter'])
            f.write('\nCounter ' + str(counter))
            f.write('\n')
            data = log['data']
            idx = 1048576
            for j in range(counter, 1048576):
                f.write(str(idx) + ' ' + hex(data[j]))
                f.write('\n')
                idx -= 1
            for j in range(0, counter):
                f.write(str(idx) + ' ' + hex(data[j]))
                f.write('\n')
                idx -= 1


# This registers our class to the gdb runtime at "source" time.
thread_log_dump()
