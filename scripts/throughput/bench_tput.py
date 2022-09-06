import os
import sys
import copy
import re
import csv
import numpy as np

sys.path.append("..")
import bench_util as util

util.args_parser.add_argument(
    '--range', type=str, nargs='?', default='0,4', help="l,r means [1<<l, 1<<r] Byte")
util.args_parser.add_argument(
    '--step', type=int, nargs='?', default=1, help="Range step")
args = util.args_parser.parse_args()
args.range = range(*map(int, args.range.split(',')), args.step)

opt = util.opt_parser.parse_args(("%s -a throughput" % args.opt).split())
os.chdir("../../")

pattern = re.compile(r"\[meter\s*?([0-9]*?)\]")
pattern_rps = re.compile(r"([0-9]*?\.?[0-9]*?)\sops/s")


def parse_result(out):
    out = out[2:]
    cnt = {}
    ops = {}
    for line in out:
        tid = re.findall(pattern, line)[0]
        rps = float(re.findall(pattern_rps, line)[0])
        if tid not in cnt:
            cnt[tid] = 0
            ops[tid] = []
        cnt[tid] += 1
        if cnt[tid] >= 2:
            ops[tid].append(rps)
    n = 1000000
    for tid in ops:
        n = min(n, len(ops[tid]))
    res = []
    for i in range(0, n):
        res.append(0)
        for tid in ops:
            res[i] += ops[tid][i]
    return res


res = []
for k in args.range:
    opt.T = 1 << k
    args.envoy = False
    print("Running %d threads" % opt.T, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    out = util.run_client(args, opt)

    ops = parse_result(out)
    for x in ops:
        res.append((opt.T, x))

util.killall(args)

writer = csv.writer(sys.stdout, lineterminator='\n')
writer.writerow(['# Client Threads', 'RPC Rate (rps)', 'Solution'])
for k, y in res:
    writer.writerow([k, y / 1e6, "gRPC"])
