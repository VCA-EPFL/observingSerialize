import json
import sys
from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np

# Configuration parameters
MAX_VALUE = 400  # Maximum value threshold

def read_json_lines():
    """Read JSON lines from stdin and collect values for each field."""
    data = defaultdict(list)
    filtered_counts = defaultdict(int)
    
    for line in sys.stdin:
        try:
            json_obj = json.loads(line.strip())
            # Collect values for each field, applying threshold
            for key, value in json_obj.items():
                if value <= MAX_VALUE:
                    data[key].append(value)
                else:
                    filtered_counts[key] += 1
        except json.JSONDecodeError as e:
            print(f"Warning: Skipping invalid JSON line: {line.strip()}")
            print(f"Error: {e}")
        except Exception as e:
            print(f"Warning: Unexpected error processing line: {line.strip()}")
            print(f"Error: {e}")
    return data, filtered_counts

def create_histograms(data, filtered_counts):
    """Create histograms for each field in the data using the same bins."""
    n_fields = len(data)
    n_rows = (n_fields + 1) // 2
    
    fig, axes = plt.subplots(n_rows, 2, figsize=(15, 5*n_rows))
    fig.suptitle(f'Distribution of Values by Field (Values ≤ {MAX_VALUE})', fontsize=16, y=0.95)
    
    # Flatten axes array for easier iteration
    axes = axes.flatten() if n_fields > 2 else [axes] if n_fields == 1 else axes
    
    # Calculate global min and max for consistent bins
    all_values = []
    for values in data.values():
        all_values.extend(values)
    global_min = min(all_values)
    global_max = min(MAX_VALUE, max(all_values))
    
    # Calculate optimal number of bins using Sturge's rule on the total dataset
    n_bins = int(np.log2(len(all_values)) + 1)
    
    # Create bins that will be used for all histograms
    bins = np.linspace(global_min, global_max, n_bins + 1)
    
    # Create a histogram for each field
    for idx, (field, values) in enumerate(sorted(data.items())):
        ax = axes[idx]
        
        # Create histogram with the common bins
        counts, bins, _ = ax.hist(values, bins=bins, edgecolor='black', alpha=0.7)
        
        # Add mean and median lines
        mean_val = np.mean(values)
        median_val = np.median(values)
        ax.axvline(mean_val, color='red', linestyle='dashed', linewidth=2, label=f'Mean: {mean_val:.2f}')
        ax.axvline(median_val, color='green', linestyle='dashed', linewidth=2, label=f'Median: {median_val:.2f}')
        
        # Create title with filtered count information
        filtered_msg = f"\nFiltered out: {filtered_counts[field]} values > {MAX_VALUE}" if filtered_counts[field] > 0 else ""
        ax.set_title(f'Distribution of {field}\n(n={len(values)}{filtered_msg})')
        
        ax.set_xlabel('Value')
        ax.set_ylabel('Frequency')
        ax.grid(True, alpha=0.3)
        ax.legend()
        
        # Add value annotations above each bar
        for i in range(len(counts)):
            if counts[i] > 0:  # Only annotate non-empty bars
                ax.text(bins[i], counts[i], str(int(counts[i])), 
                       horizontalalignment='center', verticalalignment='bottom')
        
        # Set the same x-axis limits for all plots
        ax.set_xlim(global_min - (global_max - global_min)*0.05, 
                   global_max + (global_max - global_min)*0.05)
    
    # Hide empty subplots if any
    for idx in range(len(data), len(axes)):
        axes[idx].set_visible(False)
    
    # Adjust layout to prevent overlap
    plt.tight_layout()
    return fig

def print_statistics(data, filtered_counts):
    """Print basic statistics for each field."""
    print("\nBasic Statistics:")
    print("-" * 80)
    print(f"{'Field':<15} {'Count':>8} {'Filtered':>8} {'Mean':>10} {'Median':>10} {'Min':>8} {'Max':>8}")
    print("-" * 80)
    
    for field, values in sorted(data.items()):
        print(f"{field:<15} {len(values):8d} {filtered_counts[field]:8d} {np.mean(values):10.2f} "
              f"{np.median(values):10.2f} {min(values):8d} {max(values):8d}")

def main():
    print(f"Reading JSON data from stdin... (filtering values > {MAX_VALUE})")
    data, filtered_counts = read_json_lines()
    
    if not data:
        print("No valid JSON data received!")
        sys.exit(1)
    
    # Print statistics
    print_statistics(data, filtered_counts)
    
    # Create and save histograms
    fig = create_histograms(data, filtered_counts)
    output_file = 'distributions.png'
    fig.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"\nHistograms saved to: {output_file}")
    
    # Close the figure to free memory
    plt.close(fig)

if __name__ == "__main__":
    main()
