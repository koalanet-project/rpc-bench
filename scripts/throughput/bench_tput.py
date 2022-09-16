import os
import sys
import copy
import re
import csv
import numpy as np

sys.path.append("..")
import bench_util as util

util.args_parser.add_argument(
    '--range', type=str, nargs='?', default='0,4', help="l,r means [1<<l, 1<<r] threads")
util.args_parser.add_argument(
    '--step', type=int, nargs='?', default=1, help="Range step")

args = util.args_parser.parse_args()
args.range = range(*map(int, args.range.split(',')), args.step)

opt = util.opt_parser.parse_args(("%s -a throughput -t 10" % args.opt).split())
os.chdir("../../")

pattern_tid = re.compile(r"\[meter\s*?([0-9]*?)\]")
pattern_rps = re.compile(r"([0-9]*?\.?[0-9]*?)\sops/s")
pattern_cpu = re.compile(r"CPU utilization:\s([0-9]*?\.?[0-9]*?)%")


def parse_result(out):
    print(out)
    out = out.split('\n')
    cnt = {}
    ops = {}
    cpus = []
    for line in out:
        tid = re.findall(pattern_tid, line)
        rps = re.findall(pattern_rps, line)
        if len(tid) > 0 and len(rps) > 0:
            tid = int(tid[0])
            rps = float(rps[0])
            if tid not in cnt:
                cnt[tid] = 0
                ops[tid] = []
            cnt[tid] += 1
            if cnt[tid] >= 2:
                ops[tid].append(rps)
        cpu = re.findall(pattern_cpu, line)
        if len(cpu) > 0:
            cpu = float(cpu[0])
            cpus.append(cpu)
    cpus = cpus[1:-1]
    n = len(cpus)
    for tid in ops:
        n = min(n, len(ops[tid]))
    rate = [0] * n
    for i in range(0, n):
        for tid in ops:
            rate[i] += ops[tid][i]
    print('rate', rate)
    print('cpu util', cpus)
    return rate, cpus


res = []
for k in args.range:
    opt.T = 1 << k
    args.envoy = False
    print("Running %d threads" % opt.T, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    fname = util.run_cpu_monitor(args, f"tput_{opt.T}t_{opt.C}c")
    out = util.run_client(args, opt)
    mpstat_srv, mpstat_cli = util.stop_cpu_monitor(args, fname)

    rates, cpus = parse_result(out)
    cpus_srv = mpstat_srv[-1 - len(rates):-1]
    cpus_cli = mpstat_cli[-1 - len(rates):-1]
    print("mpstat server", cpus_srv)
    print("mpstat client", cpus_cli)

    cpus_srv = [sum(stat) for stat in cpus_srv]  # use mpstat instead
    cpus_cli = [sum(stat) for stat in cpus_cli]  # use mpstat instead
    for y1, y2, y3 in zip(rates, cpus_srv, cpus_cli):
        res.append((opt.T, y1, y2, y3))

res_envoy = []
for k in args.range:
    opt.T = 1 << k
    args.envoy = True
    print("Running %d threads with envoy" % opt.T, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    opt_client = copy.deepcopy(opt)
    opt_client.p = 10001  # envoy port
    fname = util.run_cpu_monitor(args, f"tput_{opt.T}t_{opt.C}c")
    out = util.run_client(args, opt_client)
    mpstat_srv, mpstat_cli = util.stop_cpu_monitor(args, fname)

    rates, cpus = parse_result(out)
    cpus_srv = mpstat_srv[-1 - len(rates):-1]
    cpus_cli = mpstat_cli[-1 - len(rates):-1]
    print("mpstat server", cpus_srv)
    print("mpstat client", cpus_cli)

    cpus_srv = [sum(stat) for stat in cpus_srv]  # use mpstat instead
    cpus_cli = [sum(stat) for stat in cpus_cli]  # use mpstat instead
    for y1, y2, y3 in zip(rates, cpus_srv, cpus_cli):
        res_envoy.append((opt.T, y1, y2, y3))

util.killall(args)

fname = f"/tmp/rpc_bench_tput_{opt.C}c"
with open(fname, "w") as fout:
    writer = csv.writer(fout, lineterminator='\n')
    writer.writerow(['# User Threads', 'RPC Rate (Mrps)',
                    'Solution', 'Server CPU', 'Client CPU'])
    for k, y1, y2, y3 in res:
        writer.writerow([k, float(format(y1 / 1e6, '.3g')),
                        "gRPC", round(y2 / 1e2, 3), round(y3 / 1e2, 3)])
    for k, y1, y2, y3 in res_envoy:
        writer.writerow([k, float(format(y1 / 1e6, '.3g')),
                        "gRPC+Envoy", round(y2 / 1e2, 3), round(y3 / 1e2, 3)])
print(fname)
with open(fname, "r") as fin:
    print(fin.read())
