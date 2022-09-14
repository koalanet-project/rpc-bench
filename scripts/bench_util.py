import os
import sys
import signal
import subprocess
import argparse
import time

args_parser = argparse.ArgumentParser()
args_parser.add_argument('server', metavar='server',
                         type=str, nargs='?', default='rdma0.danyang-06')
args_parser.add_argument(
    '--opt', type=str, nargs='?', default='', help="Options of rpc-bench")
args_parser.add_argument(
    '-u', action='store_true', help="Monitor CPU utilization at the client (default at the server)")

opt_parser = argparse.ArgumentParser()
opt_parser.add_argument('-a', type=str, nargs='?')
opt_parser.add_argument('-p', type=int, nargs='?', default=18000)
opt_parser.add_argument('-t', type=int, nargs='?', default=10)
opt_parser.add_argument('-c', type=int, nargs='?', default=32)
opt_parser.add_argument('-T', type=int, nargs='?', default=1)
opt_parser.add_argument('-d', type=int, nargs='?', default=32)


def ssh_cmd(server, cmd):
    os.system('ssh -o "StrictHostKeyChecking no" %s "%s"' % (server, cmd))


def killall(args):
    cmd = "pkill -9 rpc-bench"
    os.system(cmd)
    ssh_cmd(args.server, cmd)

    cmd = "pkill -9 envoy"
    os.system(cmd)
    ssh_cmd(args.server, cmd)

    cmd = "pkill -9 mpstat"
    os.system(cmd)
    ssh_cmd(args.server, cmd)

    time.sleep(1)


def run_server(args, opt):
    envoy_cmd = ""
    print(f'enable Envoy: {args.envoy}')
    if args.envoy:
        envoy_cmd = "nohup envoy -c envoy/envoy.yaml >scripts/%s/envoy_server.log 2>&1 &" % opt.a
    server_script = '''
    cd ~/nfs/Developing/rpc-bench; %s;
    GLOG_minloglevel=2 nohup ./build/rpc-bench -a %s -r grpc -d %d -T %d >/dev/null 2>&1 &
    ''' % (envoy_cmd, opt.a, opt.d, opt.T)
    print('server envoy:', envoy_cmd)
    print('server:', server_script)
    ssh_cmd(args.server, server_script)
    time.sleep(2)


def run_client(args, opt):
    envoy_cmd = ""
    env = dict(os.environ, GLOG_minloglevel="2", GLOG_logtostderr="1")
    print(f'enable Envoy: {args.envoy}')
    if args.envoy:
        envoy_cmd = "nohup envoy -c envoy/envoy.yaml >scripts/%s/envoy_client.log 2>&1 &" % opt.a
        env = dict(env, http_proxy="http://127.0.0.1:10000/",
                   grpc_proxy="http://127.0.0.1:10000/")
    print('client envoy:', envoy_cmd)
    os.system(envoy_cmd)
    time.sleep(1)
    client_script = '''
    ./build/rpc-bench -a %s -r grpc -d %d -C %d -T %d -p %d -t %d --monitor-time=%d %s
    ''' % (opt.a, opt.d, opt.c, opt.T, opt.p, opt.t, opt.t, args.server)
    print('client:', client_script)
    with subprocess.Popen(client_script.split(), env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
        out, err = proc.communicate()
    out = out.decode().strip()
    err = err.decode().strip()
    return err + out


def run_cpu_monitor(args):
    fout = "/tmp/rpc_bench_cpu_monitor_%d" % time.time_ns()
    # global proc_cpu
    # proc_cpu = subprocess.Popen("mpstat -P ALL -u 1 | grep --line-buffered 'all'",
    #                             shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    #                             preexec_fn=os.setsid)
    cmd = f"nohup sh -c 'mpstat -P ALL -u 1 | grep --line-buffered all' >{fout} 2>/dev/null &"
    print("CPU monitor:", cmd)
    if args.u:
        os.system(cmd)
    else:
        ssh_cmd(args.server, cmd)
    return fout


def stop_cpu_monitor(args, fname):
    raw_output = ""
    killcmd = "pkill -9 mpstat"
    if args.u:
        os.system(killcmd)
        with open(fname, "r") as fout:
            raw_output = fout.read().strip()
    else:
        ssh_cmd(args.server, killcmd)
        with subprocess.Popen(f'ssh -o "StrictHostKeyChecking no" {args.server} "cat {fname}"', shell=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE) as proc:
            out, err = proc.communicate()
            raw_output = out.decode().strip()
    cpus = []
    for row in raw_output.split('\n'):
        line = row.split()
        utime = float(line[3]) * 40.0
        stime = float(line[5]) * 40.0
        cpus.append(utime + stime)
    return cpus
