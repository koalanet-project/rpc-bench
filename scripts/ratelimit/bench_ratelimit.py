import os
import sys
import copy
import re
import csv
import time
import numpy as np

sys.path.append("..")
import bench_util as util

args = util.args_parser.parse_args()
args.timestamp = time.time_ns()

opt = util.opt_parser.parse_args(("%s -a throughput -t 13" % args.opt).split())
os.chdir("../../")

pattern_mbps = re.compile(r"([0-9]*?\.?[0-9]*?)\sMb/s")
pattern_rps = re.compile(r"([0-9]*?\.?[0-9]*?)\sops/s")


def parse_result(out):
    print(out)
    bw = re.findall(pattern_mbps, out)
    print('bw', bw)
    ops = re.findall(pattern_rps, out)
    print('rate', ops)
    return [float(i) for i in bw[3:-1]], [float(i) for i in ops[3:-1]]


args.envoy = False
util.killall(args)
util.run_server(args, opt)
out = util.run_client(args, opt)
goodputs, ops = parse_result(out)


args.envoy = True
util.killall(args)
util.run_server(args, opt)
opt_client = copy.deepcopy(opt)
opt_client.p = 10001  # envoy port
out = util.run_client(args, opt_client)
goodputs_envoy, ops_envoy = parse_result(out)

util.killall(args)

fname = f"/tmp/rpc_bench_ratelimit_{args.timestamp}.csv"
with open(fname, "w") as fout:
    writer = csv.writer(fout, lineterminator='\n')
    writer.writerow(['RPC Rate (Krps)', 'Solution', 'Type'])
    for y in ops:
        writer.writerow([round(y / 1000, 2), 'gRPC', 'w/o Limit'])
    for y in ops_envoy:
        writer.writerow([round(y / 1000, 2), 'gRPC', 'w/ Limit'])

print(fname)
with open(fname, "r") as fin:
    print(fin.read())
