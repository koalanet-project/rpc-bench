import os
import sys
import copy
import re
import csv
import time

sys.path.append("..")
import bench_util as util

util.args_parser.add_argument(
    '--range', type=str, nargs='?', default='1,14', help="l,r means [1<<l, 1<<r) KByte")
util.args_parser.add_argument(
    '--step', type=int, nargs='?', default=2, help="Range step")
args = util.args_parser.parse_args()
args.range = range(*map(int, args.range.split(',')), args.step)

opt = util.opt_parser.parse_args(("%s -a throughput" % args.opt).split())
os.chdir("../../")

# pattern = re.compile(r"([0-9]*?\.?[0-9]*?)\sGbps")
pattern_mbps = re.compile(r"([0-9]*?\.?[0-9]*?)\sMb/s")
pattern_rps = re.compile(r"([0-9]*?\.?[0-9]*?)\sops/s")

args.timestamp = time.time_ns()


def parse_result(out):
    print(out)
    bw = re.findall(pattern_mbps, out)
    print('bw', bw)
    ops = re.findall(pattern_rps, out)
    print('rate', ops)
    return [float(i) for i in bw[1:-1]], [float(i) for i in ops[1:-1]]


res = []
for k in args.range:
    args.envoy = False
    opt.d = (1 << k) * 1024
    print("Running size %d" % opt.d, file=sys.stderr)
    util.killall(args)
    opt.d = 32
    util.run_server(args, opt)
    opt.d = (1 << k) * 1024
    fname = util.run_cpu_monitor(args, f"bw_{opt.d}b_{opt.C}c")
    out = util.run_client(args, opt)
    mpstat_srv, mpstat_cli = util.stop_cpu_monitor(args, fname)

    goodputs, ops = parse_result(out)

    cpus_srv, cpus_cli = util.merge_mpstat(mpstat_srv, mpstat_cli, len(ops))
    for y1, y2, y3 in zip(goodputs, cpus_srv, cpus_cli):
        res.append((round(opt.d / 1024), y1, y2, y3))

res_envoy = []
for k in args.range:
    args.envoy = True
    opt.d = (1 << k) * 1024
    print("Running size %d with envoy" % opt.d, file=sys.stderr)
    util.killall(args)
    opt.d = 32
    util.run_server(args, opt)
    opt.d = (1 << k) * 1024
    opt_client = copy.deepcopy(opt)
    opt_client.p = 10001  # envoy port
    fname = util.run_cpu_monitor(args, f"bw_{opt.d}b_{opt.C}c")
    out = util.run_client(args, opt_client)
    mpstat_srv, mpstat_cli = util.stop_cpu_monitor(args, fname)

    goodputs, ops = parse_result(out)

    cpus_srv, cpus_cli = util.merge_mpstat(mpstat_srv, mpstat_cli, len(ops))
    for y1, y2, y3 in zip(goodputs, cpus_srv, cpus_cli):
        res_envoy.append((round(opt.d / 1024), y1, y2, y3))

util.killall(args)


fname = f"/tmp/rpc_bench_bw_{opt.C}c_{args.timestamp}.csv"
with open(fname, "w") as fout:
    writer = csv.writer(fout, lineterminator='\n')
    writer.writerow(['RPC Size (KB)', 'Goodput (Gbps)',
                    'Solution', 'Server CPU', 'Client CPU'])
    for k, y1, y2, y3 in res:
        writer.writerow([k, round(y1 / 1e3, 3),
                        "gRPC", round(y2 / 1e2, 3), round(y3 / 1e2, 3)])
    for k, y1, y2, y3 in res_envoy:
        writer.writerow([k, round(y1 / 1e3, 3),
                        "gRPC+Envoy", round(y2 / 1e2, 3), round(y3 / 1e2, 3)])
print(fname)
with open(fname, "r") as fin:
    print(fin.read())
