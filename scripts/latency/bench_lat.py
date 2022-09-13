import os
import sys
import copy
import csv

sys.path.append("..")
import bench_util as util

util.args_parser.add_argument(
    '--range', type=str, nargs='?', default='6,11', help="l,r means [1<<l, 1<<r] Byte")
util.args_parser.add_argument(
    '--step', type=int, nargs='?', default=1, help="Range step")
args = util.args_parser.parse_args()
args.range = range(*map(int, args.range.split(',')), args.step)

opt = util.opt_parser.parse_args(("%s -a latency" % args.opt).split())
os.chdir("../../")

res = []
for k in args.range:
    args.envoy = False
    opt.d = 1 << k
    print("Running size %d" % opt.d, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    out = util.run_client(args, opt)

    print(out)
    items = out.split('\n')[-1].split()
    res.append(float(items[1]))

res_envoy = []
for k in args.range:
    args.envoy = True
    opt.d = 1 << k
    print("Running size %d with envoy" % opt.d, file=sys.stderr)
    util.killall(args)
    util.run_server(args, opt)
    opt_client = copy.deepcopy(opt)
    opt_client.p = 10001  # envoy port
    out = util.run_client(args, opt_client)

    print(out)
    items = out.split('\n')[-1].split()
    res_envoy.append(float(items[1]))

util.killall(args)


writer = csv.writer(sys.stdout, lineterminator='\n')
writer.writerow(['Message Size (Byte)', 'Latency (us)', 'Solution'])
for k, y in zip(args.range, res):
    writer.writerow([1 << k, y, "gRPC"])
for k, y in zip(args.range, res_envoy):
    writer.writerow([1 << k, y, "gRPC (envoy)"])
