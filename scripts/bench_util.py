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

    os.system("pkill -9 mpstat")  # only client will run mpstat

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
    err = err.decode().strip()
    out = out.decode().strip()
    return err + out


proc_cpu = None


def run_cpu_monitor():
    global proc_cpu
    proc_cpu = subprocess.Popen("mpstat -P ALL -u 1 | grep --line-buffered 'all'",
                                shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                preexec_fn=os.setsid)


def stop_cpu_monitor():
    os.killpg(os.getpgid(proc_cpu.pid), signal.SIGKILL)
    # proc_cpu.kill()
    out, err = proc_cpu.communicate()
    out = out.decode().strip()
    err = err.decode().strip()
    cpus = []
    for row in out.split('\n'):
        line = row.split()
        utime = float(line[3]) * 40.0
        systime = float(line[5]) * 40.0
        cpus.append(utime)
    return cpus
