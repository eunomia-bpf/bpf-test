import matplotlib.pyplot as plt
import numpy as np
import json

structure_data = ""

with open("test-data-multi-without-smatrt-ptr.json", "r") as f:
    structure_data = json.load(f)
    
print(structure_data)

https_structure = structure_data["https"]
http_structure = structure_data["http"]

# Function to calculate the averages for 'request' and 'transfer' for each size
def calculate_averages(data):
    averages = {}
    for sub_key in data:
        size_sum = {}
        size_count = {}

        # Process each list of entries
        for entry_list in data[sub_key]['details']:
            for item in entry_list:
                size = item['size']
                request = item['request']
                transfer = item['transfer']

                if size not in size_sum:
                    size_sum[size] = {'request': 0, 'transfer': 0}
                    size_count[size] = 0

                size_sum[size]['request'] += request
                size_sum[size]['transfer'] += transfer
                size_count[size] += 1

        # Calculate averages
        averages[sub_key] = {}
        for size in size_sum:
            averages[sub_key][size] = {
                'average_request': size_sum[size]['request'] / size_count[size],
                'average_transfer': size_sum[size]['transfer'] / size_count[size]
            }

    return averages

# Calculating averages for 'https' and 'http'
https_averages = calculate_averages(https_structure)
http_averages = calculate_averages(http_structure)

def calculate_performance_drop(averages, baseline_key):
    performance_drop = {}
    baseline = averages[baseline_key]

    for sub_key in averages:
        if sub_key == baseline_key:
            continue  # Skipping the baseline itself

        performance_drop[sub_key] = {}
        for size in baseline:
            baseline_request = baseline[size]["average_request"]
            baseline_transfer = baseline[size]["average_transfer"]

            current_request = averages[sub_key][size]["average_request"]
            current_transfer = averages[sub_key][size]["average_transfer"]

            request_drop = (
                ((baseline_request - current_request) / baseline_request) * 100
                if baseline_request
                else 0
            )
            transfer_drop = (
                ((baseline_transfer - current_transfer) / baseline_transfer) * 100
                if baseline_transfer
                else 0
            )

            performance_drop[sub_key][size] = {
                "request_drop": request_drop,
                "transfer_drop": transfer_drop,
            }

    return performance_drop

# Calculating performance drop for HTTPS and HTTP
https_performance_drop = calculate_performance_drop(https_averages, "no-probe")
http_performance_drop = calculate_performance_drop(http_averages, "no-probe")

# Helper function to plot the performance drop
def plot_request_performance_drop(performance_drop, title, filename):
    sizes = sorted(list(performance_drop[next(iter(performance_drop))].keys()))
    # Assuming 'probe' and 'uprobes' are the only keys besides 'no-probe'
    legend_labels = ['Deepflow', 'Deepflow-kernel', 'Deepflow-UserBPF']
    linewidth = 3  # Thicker line width

    # Plotting Request Drops
    plt.figure(figsize=(6, 6))
    i = 0  # Index for legend labels
    for sub_key in performance_drop:
        request_drops = [performance_drop[sub_key][size]["request_drop"] for size in sizes]

        # Assign labels based on index
        if i < len(legend_labels) and i != 1 :
            label = legend_labels[i]
        # else:
            # label = f'Series {i}'  # Fallback if there are more keys than expected

            plt.plot(sizes, request_drops, label=label, linewidth=linewidth)
        i += 1

    plt.xlabel("Size", fontsize=22)
    plt.ylabel("Request Drop (%)", fontsize=22)
    plt.legend(fontsize=22)
    plt.tick_params(axis="both", which="major", labelsize=22)
    # plt.title(title, fontsize=22)

    # Show and save the plot
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(filename, format='pdf') 
    plt.show()
    plt.close()

# Call the function to plot for HTTPS and HTTP
plot_request_performance_drop(https_performance_drop, "HTTPS Request Performance Drop", "https-request-performance-drop.pdf")
plot_request_performance_drop(http_performance_drop, "HTTP Request Performance Drop", "http-request-performance-drop.pdf")
