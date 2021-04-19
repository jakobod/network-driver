#!/usr/bin/env python3
import argparse
import pandas as pd
import matplotlib.pyplot as plt


def plot_or_show(output_path):
  if output_path:
    plt.savefig(output_path,
                bbox_inches='tight', transparent=True)
    print('wrote', output_path)
  else:
    plt.show()


def y_formatter(y, pos):
  if y >= 1_000_000_000:
    return '{:>}G'.format(float(y / 1_000_000_000))
  if y >= 1_000_000:
    return '{:>}M'.format(float(y / 1_000_000))
  elif y >= 1_000:
    return '{:>}K'.format(float(y / 1_000))
  return '{:>}'.format(int(y))


def parse(input_path, output_path, title):
  df = pd.read_csv(input_path)
  print(df.columns)
  df.drop([' num_read_events', ' num_write_events',
           ' num_disconnects'], 1, inplace=True)
  df = df.mean(axis=0)
  print(df)
  ax = df.plot.bar(rot=0)
  ax.yaxis.set_major_formatter(plt.FuncFormatter(y_formatter))
  if title:
    plt.title(title, loc='left')
  plt.ylabel('Throughput [Byte/sec]')
  plt.xlabel('')
  plot_or_show(output_path)


def copy_mirror(input_path, output_path, title):
  df_1 = pd.read_csv(input_path+'copy_mirror_1_client.csv')
  df_10 = pd.read_csv(input_path+'copy_mirror_10_client.csv')
  df_100 = pd.read_csv(input_path+'copy_mirror_100_client.csv')
  df_1.drop([' num_read_events', ' num_write_events',
             ' num_disconnects'], 1, inplace=True)
  df_10.drop([' num_read_events', ' num_write_events',
              ' num_disconnects'], 1, inplace=True)
  df_100.drop([' num_read_events', ' num_write_events',
               ' num_disconnects'], 1, inplace=True)
  print(df_100.columns)
  df_1 = df_1.mean(axis=0)
  df_10 = df_10.mean(axis=0)
  df_100 = df_100.mean(axis=0)
  result = pd.concat([df_1, df_10, df_100], keys=[
                     '1 client', '10 clients', '100 clients'])
  print(result)
  result.plot.bar(rot=0)
  plt.show()


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      'input', help='The path from which the files should be read', metavar='INPUT_PATH')
  parser.add_argument(
      '--output', help='Path at which created files are written to', metavar='OUTPUT_PATH')
  parser.add_argument(
      '--title', help='Title of the resulting Plot', metavar='TITLE')
  args = parser.parse_args()
  parse(args.input, args.output, args.title)


if __name__ == '__main__':
  main()
