import matplotlib
matplotlib.use('Agg')

import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
from matplotlib.ticker import FuncFormatter, MultipleLocator

paper_rc = {'lines.linewidth': 3, 'lines.markersize': 15}
sns.set(context='paper',
        font_scale=2.2,
        style="ticks",
        palette="Paired",
        rc=paper_rc)

hue_order = ["mRPC", "eRPC", "gRPC", "gRPC (envoy)"]
palette = ["blue", "red", "purple", "pink"]
markers = ['.', '*', '^', 'v']
dashes = sns._core.unique_dashes(10)

df = pd.concat([pd.read_csv("bench_bw.csv"),
               pd.read_csv("mrpc_bw.csv")], ignore_index=True)

g = sns.lineplot(
    data=df,
    x='Message Size (KB)',
    y='Bandwidth (Gb/s)',
    hue='Solution',
    hue_order=hue_order,
    markers=dict(zip(hue_order, markers)),
    style='Solution',
    err_style='band',
    palette=dict(zip(hue_order, palette)),
    dashes=dict(zip(hue_order, dashes))
)

g.axes.set_ylim(0.01, 100)
plt.xscale('log', basex=2)
plt.yscale('log', basey=10)
g.axes.xaxis.set_major_formatter(
    FuncFormatter(lambda x, _: '{:g}'.format(x)))
g.legend(loc='upper left', bbox_to_anchor=(1, 1), title='')
# for ax in g.axes.flatten():
#     ax.tick_params(axis='y', which='both', direction='out', length=4, left=True)
#     ax.grid(b=True, which='both', color='gray', linewidth=0.1)
ml = MultipleLocator(5)
g.axes.yaxis.set_minor_locator(ml)
g.axes.yaxis.set_tick_params(which='minor')
sns.despine(fig=None, top=True, right=True, left=False, bottom=False)
plt.grid(axis='y', color='black', linestyle=':', linewidth=0.5)
xticks = list(map(lambda x: 1 << x, range(1, 13, 2)))
g.set_xticks(xticks)
g.set_xticklabels(list(map(str, xticks)))

plt.savefig('bench_bw.pdf', bbox_inches='tight')
# plt.show()
