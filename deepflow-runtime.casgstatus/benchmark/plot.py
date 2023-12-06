import matplotlib.pyplot as plt
import numpy as np
import json

structure_data = ""

with open("test-data-multi-without-smatrt-ptr.json", "r") as f:
    structure_data = json.load(f)
    
# print(structure_data)

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

print(https_averages)
print(http_averages)

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

print("\n# performance drop\n")

print(https_performance_drop)
print(http_performance_drop)

# Helper function to plot the performance drop
def plot_performance_drop(performance_drop, title):
    sizes = sorted(list(performance_drop[next(iter(performance_drop))].keys()))
    sub_keys = list(performance_drop.keys())
    request_drops = {
        sub_key: [performance_drop[sub_key][size]["request_drop"] for size in sizes]
        for sub_key in sub_keys
    }
    transfer_drops = {
        sub_key: [performance_drop[sub_key][size]["transfer_drop"] for size in sizes]
        for sub_key in sub_keys
    }

    # Plotting
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 6))
    fig.suptitle(title, fontsize=16)

    # Plot for Request Drops
    for sub_key in sub_keys:
        ax1.plot(sizes, request_drops[sub_key], label=sub_key)
    ax1.set_xlabel("Size", fontsize=14)
    ax1.set_ylabel("Request Drop (%)", fontsize=14)
    ax1.legend(fontsize=12)
    ax1.tick_params(axis="both", which="major", labelsize=12)
    ax1.set_title("Request Performance Drop", fontsize=14)

    # Plot for Transfer Drops
    for sub_key in sub_keys:
        ax2.plot(sizes, transfer_drops[sub_key], label=sub_key)
    ax2.set_xlabel("Size", fontsize=14)
    ax2.set_ylabel("Transfer Drop (%)", fontsize=14)
    ax2.legend(fontsize=12)
    ax2.tick_params(axis="both", which="major", labelsize=12)
    ax2.set_title("Transfer Performance Drop", fontsize=14)

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    return plt


# Plotting for HTTPS
plt_https = plot_performance_drop(https_performance_drop, "HTTPS Performance Drop")
plt_https.savefig("https-performance-drop.png")

# Plotting for HTTP
plt_http = plot_performance_drop(http_performance_drop, "HTTP Performance Drop")
plt_http.savefig("http-performance-drop.png")
