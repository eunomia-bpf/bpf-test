import re
import statistics
import matplotlib.pyplot as plt
import numpy as np


def get_stats_and_plot(log_file_path, label, color):
    # Regular expressions to extract the numerical values for requests per second and transfer per second
    req_sec_pattern = re.compile(r"Requests/sec:\s+(\d+\.\d+)")
    transfer_sec_pattern = re.compile(r"Transfer/sec:\s+(\d+\.\d+)MB")

    # Lists to store the values extracted from the log file
    req_sec_values = []
    transfer_sec_values = []

    # Open the log file and extract the values
    with open(log_file_path, "r") as file:
        log_contents = file.read()
        req_sec_values = [
            float(match) for match in req_sec_pattern.findall(log_contents)
        ]
        transfer_sec_values = [
            float(match) for match in transfer_sec_pattern.findall(log_contents)
        ]

    # Function to calculate various statistics
    def calculate_statistics(values):
        return {
            "mean": statistics.mean(values),
            "median": statistics.median(values),
            "stdev": statistics.stdev(values) if len(values) > 1 else 0,
            "min": min(values),
            "max": max(values),
        }

    # Calculate statistics for Requests/sec and Transfer/sec
    req_sec_stats = calculate_statistics(req_sec_values)
    transfer_sec_stats = calculate_statistics(transfer_sec_values)

    # Return the means for graphing
    return req_sec_stats["mean"], transfer_sec_stats["mean"]


# Labels and colors for each scenario
scenarios = [
    "Userspace",
    "No Syscount",
    "Kernel Filter",
    "kernel-untarget",
    "Userspace Filter",
]
colors = ["blue", "green", "red", "orange", "yellow"]

# File paths
file_paths = [
    "result/userspace.txt",
    "result/no-syscount.txt",
    "result/kernel_targeted-filter.txt",
    "result/kernel_untargeted-filter.txt",
    "result/userspace-untargeted.txt",
]

# Gather data
req_sec_means = []
transfer_sec_means = []
for file_path, label in zip(file_paths, scenarios):
    req_sec_mean, transfer_sec_mean = get_stats_and_plot(
        file_path, label, colors[scenarios.index(label)]
    )
    req_sec_means.append(int(req_sec_mean))
    transfer_sec_means.append(transfer_sec_mean)

print(req_sec_means)

groups = ["Native", "Kernel", "Userspace"]
group_data = {
    "Native": [req_sec_means[1]],
    "Kernel": [req_sec_means[3], req_sec_means[2]],
    "Userspace": [req_sec_means[4], req_sec_means[0]],
}
colors = {
    "Native": "#8ECFC9",
    "Kernel": ["#FFBE7A", "#FA7F6F"],
    "Userspace": ["#FFBE7A", "#FA7F6F"],
}
legends = {
    "#8ECFC9": "Native",
    "#FFBE7A": "Untargeted",
    "#FA7F6F": "Targeted",
}

off = [0.7, 0, -0.7]
bar_width = 0.10

# 绘制水平柱状图
fig, ax = plt.subplots()
fig.set_figwidth(fig.get_figwidth() * 1.4)
fig.set_figheight(fig.get_figheight() / 1.3 * 0.80) 

for i, (group, values) in enumerate(group_data.items()):
    # 计算每个组在y轴上的位置
    # 原先x轴分布改为y轴分布
    y_pos = [i + bar_width * j + off[i] for j in range(len(values))]

    # 获取每个柱子的颜色
    group_colors = (
        colors[group]
        if isinstance(colors[group], list)
        else [colors[group]] * len(values)
    )
    # 使用barh绘制横向条形图
    for y, val, col in zip(y_pos, values, group_colors):
        b = ax.barh(
            y,
            val,
            color=col,
            height=bar_width,
            label=legends[col]
            if legends[col] not in ax.get_legend_handles_labels()[1]
            else "",
        )
        ax.bar_label(b, [val], fontsize=9, padding=5)
        ax.set_xlim(0, 22000)

# 设置图例
ax.legend(loc="upper center", bbox_to_anchor=(0.5, 1.2), fontsize=10, ncol=3)

# 原先的x轴为分组名，现在y轴为分组名
ax.set_yticks([r + off[r] for r in range(len(groups))])
ax.set_yticklabels(groups, fontsize=10)

# x轴显示请求数，y轴显示场景
plt.xlabel("Requests per Second (RPS)", fontsize=12, labelpad=10)
plt.ylabel("Scenario", fontsize=12, labelpad=10)
plt.xticks(fontsize=10)

# 设置x轴的刻度标签
ax.set_xticks([r + off[r] for r in range(len(groups))])
ax.set_xticklabels(groups, fontsize=15)
# plt.title('Average Requests per Second by Scenario')
plt.xlabel("Scenario", fontsize=20, labelpad=-1)
plt.ylabel("Requests per Second (RPS)", fontsize=20, labelpad=-7)
plt.savefig("syscount-req.pdf")
plt.tight_layout()
plt.savefig("syscount-req-horizontal.pdf")
plt.show()
