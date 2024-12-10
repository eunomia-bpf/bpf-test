import re
import statistics
import matplotlib.pyplot as plt
import numpy as np

def get_stats_and_plot(log_file_path, label, color):
    # Regular expressions to extract the numerical values for requests per second and transfer per second
    req_sec_pattern = re.compile(r"Requests/sec:\s+(\d+\.\d+)")
    transfer_sec_pattern = re.compile(r"Transfer/sec:\s+(\d+\.\d+)MB")

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
colors_scenarios = ["blue", "green", "red", "orange", "yellow"]

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
        file_path, label, colors_scenarios[scenarios.index(label)]
    )
    req_sec_means.append(int(req_sec_mean))
    transfer_sec_means.append(transfer_sec_mean)

print(req_sec_means)

# Update the groups to rename "Userspace" to "userBPF"
# The order of scenarios is:
# Index: scenario
# 0: Userspace
# 1: No Syscount (Native)
# 2: Kernel Filter (targeted)
# 3: kernel-untarget (untargeted)
# 4: Userspace Filter (also userBPF untargeted)

groups = ["Native", "Kernel", "userBPF"]

# Assign the corresponding means to groups. Using the same indices as before:
# Native: no-syscount (index 1)
# Kernel: kernel_untargeted (index 3), kernel_targeted (index 2)
# userBPF: userspace-untargeted (index 4), userspace (index 0)
group_data = {
    "Native": [req_sec_means[1]],  # No Syscount
    "Kernel": [req_sec_means[3], req_sec_means[2]],  # untargeted, targeted
    "userBPF": [req_sec_means[4], req_sec_means[0]]  # untargeted, targeted
}

# Update colors to reflect the new naming. 
colors = {
    "Native": "#8ECFC9",
    "Kernel": ["#FFBE7A", "#FA7F6F"],
    "userBPF": ["#FFBE7A", "#FA7F6F"],  # Reusing same colors for untargeted & targeted
}

legends = {
    "#8ECFC9": "Native",
    "#FFBE7A": "Untargeted",
    "#FA7F6F": "Targeted",
}

# We'll create vertical bars now.
fig, ax = plt.subplots()

# Increase figure size for better readability
fig.set_figwidth(fig.get_figwidth())
fig.set_figheight(fig.get_figheight())

# Number of groups
num_groups = len(groups)
x = np.arange(num_groups)
bar_width = 0.3

# For each group, we may have one or two bars. 
# If one bar, it will be centered at x[i]. 
# If two bars, we place them at x[i] - bar_width/2 and x[i] + bar_width/2.
for i, (group, values) in enumerate(group_data.items()):
    group_colors = (colors[group] if isinstance(colors[group], list) else [colors[group]])
    
    if len(values) == 1:
        # Only one bar for this group, center it
        b = ax.bar(x[i], values[0], width=bar_width, color=group_colors[0])
        ax.bar_label(b, [values[0]], fontsize=12, padding=5)
        # Add legend label if not already present
        if legends[group_colors[0]] not in ax.get_legend_handles_labels()[1]:
            b.set_label(legends[group_colors[0]])
    else:
        # Two bars for this group
        left_x = x[i] - bar_width/2
        right_x = x[i] + bar_width/2
        # Untargeted vs targeted order assumed based on the original logic
        for val, col, xpos in zip(values, group_colors, [left_x, right_x]):
            b = ax.bar(xpos, val, width=bar_width, color=col)
            ax.bar_label(b, [val], fontsize=12, padding=5)
            # Add legend only if not present
            if legends[col] not in ax.get_legend_handles_labels()[1]:
                b.set_label(legends[col])

# Set legend with larger font size
ax.legend(loc="upper center", bbox_to_anchor=(0.5, 1.2), fontsize=14, ncol=3)

# Set x-ticks and labels
ax.set_xticks(x)
ax.set_xticklabels(groups, fontsize=15)

# Increase axis label and tick sizes
plt.xticks(fontsize=15)
plt.yticks(fontsize=15)

# Update axis labels with larger font size
# plt.xlabel("Scenario", fontsize=20, labelpad=10)
plt.ylabel("Requests per Second (RPS)", fontsize=20, labelpad=10)

# Set a suitable y-axis limit if desired
ax.set_ylim(0, 22000)

plt.tight_layout()

# Save the figure
plt.savefig("syscount.pdf")
plt.savefig("syscount-req.png")
plt.savefig("syscount-req-vertical.pdf")

plt.show()
