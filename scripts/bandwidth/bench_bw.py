import os
import sys
import copy
import re
import csv

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

pattern = re.compile(r"([0-9]*?\.?[0-9]*?)\sGbps")
pattern_mbps = re.compile(r"([0-9]*?\.?[0-9]*?)\sMb/s")
pattern_rps = re.compile(r"([0-9]*?\.?[0-9]*?)\sops/s")

def parse_result(out, goodputs, rates):
    print(out)

    bw = re.findall(pattern_mbps, '\n'.join(out))
    print('bw', bw)
    ops = re.findall(pattern_rps, '\n'.join(out))
    print('rate', ops)
    goodputs.append([float(i) for i in bw[1:]])
    rates.append([float(i) for i in rates[1:]])

goodputs = []
rates = []

for k in args.range:
    args.envoy = False
    opt.d = (1 << k) * 1024
    print("Running size %d" % opt.d, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    out = util.run_client(args, opt)
    parse_result(out, goodputs, rates)

goodputs_envoy = []
rates_envoy = []
for k in args.range:
    args.envoy = True
    opt.d = (1 << k) * 1024
    print("Running size %d with envoy" % opt.d, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    opt_client = copy.deepcopy(opt)
    opt_client.p = 10001  # envoy port
    out = util.run_client(args, opt_client)
    parse_result(out, goodputs_envoy, rates_envoy)

util.killall(args)

writer = csv.writer(sys.stdout, lineterminator='\n')
writer.writerow(['RPC Size (KB)', 'Goodput (Gb/s)', 'Solution'])
for k, ys in zip(args.range, goodputs):
    for y in ys:
        writer.writerow([1 << k, y * 1e-3, f"gRPC ({opt.c})"])
for k, ys in zip(args.range, goodputs_envoy):
    for y in ys:
        writer.writerow([1 << k, y * 1e-3, f"gRPC+Envoy ({opt.c})"])
