#!/usr/bin/env python3

import argparse
import re

from enum import Enum

class UpdateType(Enum):
    INIT = 1
    TREE = 2
    TREE_LOGGING = 3
    UNKNOWN = 4

class Update:
    def __init__(self, update_type, line):
        self.type = update_type
        self.line = line

        if self.type == UpdateType.INIT:
            self.init_init(line)
        elif self.type == UpdateType.TREE:
            self.init_tree(line)
        elif self.type == UpdateType.TREE_LOGGING:
            self.init_tree_logging(line)

    def init_init(self, line):
        matches = re.search('stable_timestamp=\((\d+), (\d+)\)', line)
        if matches is None:
            raise Exception("failed to parse init string")
        self.stable_txn_id = int(matches.group(1))
        self.stable_ts = int(matches.group(2))

    def init_tree(self, line):
        matches = re.search('modified=(\w+).*durable_timestamp=\((\d+), (\d+)\).*>.*stable_timestamp=\((\d+), (\d+)\): (\w+).*has_prepared_updates=(\w+).*durable_timestamp_not_found=(\w+).*txnid=(\d+).*recovery_checkpoint_snap_min=(\d+): (\w+)', line)
        if matches is None:
            raise Exception("failed to parse tree string")

        self.modified = matches.group(1).lower() == "true"

        self.durable_txn_id = int(matches.group(2))
        self.durable_ts = int(matches.group(3))
        self.stable_txn_id = int(matches.group(4))
        self.stable_ts = int(matches.group(5))
        self.durable_gt_stable = matches.group(6).lower() == "true"

        self.has_prepared_updates = matches.group(7).lower() == "true"

        self.durable_ts_not_found = matches.group(8).lower() == "true"

        self.txnid = int(matches.group(9))
        self.recovery_ckpt_snap_min = int(matches.group(10))
        self.txnid_gt_recov_ckpt_snap_min = matches.group(11).lower() == "true"

    def init_tree_logging(self, line):
        matches = re.search('connection_logging_enabled=(\w+).*btree_logging_enabled=(\w+)', line)
        if matches is None:
            raise Exception("failed to parse tree logging string")

        self.conn_logging_enabled = matches.group(1).lower() == "true"
        self.btree_logging_enabled = matches.group(2).lower() == "true"

class Checker:
    def __init__(self):
        self.stable_txn_id = None
        self.stable_ts = None

    def apply(self, update):
        if update.type == UpdateType.INIT:
            self.apply_check_init(update)
        elif update.type == UpdateType.TREE:
            self.apply_check_tree(update)
        elif update.type == UpdateType.TREE_LOGGING:
            self.apply_check_tree_logging(update)
        else:
            raise Exception(f"failed to parse {update.line}")

    def apply_check_init(self, update):
        if self.stable_txn_id is not None:
            raise Exception("restarted RTS?!")
        if self.stable_ts is not None:
            raise Exception("restarted RTS?!")

        self.stable_txn_id = update.stable_txn_id
        self.stable_ts = update.stable_ts

    def apply_check_tree(self, update):
        pass

    def apply_check_tree_logging(self, update):
        pass

def parse_to_update(line):
    if '[INIT]' in line:
        return Update(UpdateType.INIT, line)
    elif '[TREE]' in line:
        return Update(UpdateType.TREE, line)
    elif '[TREE_LOGGING]' in line:
        return Update(UpdateType.TREE_LOGGING, line)
    else:
        return Update(UpdateType.UNKNOWN, line)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Verify actions taken by rollback to stable from verbose messages.')
    parser.add_argument('file', type=str, help='the log file to parse verbose messages from')
    args = parser.parse_args()

    checker = Checker()
    with open(args.file) as f:
        for line in f:
            if 'WT_VERB_RTS' in line:
                update = parse_to_update(line)
                checker.apply(update)
