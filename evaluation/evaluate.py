#!/usr/bin/env python3
import argparse
import pandas as pd
import matplotlib.pyplot as plt


def parse(input_path, output_path):
  df = pd.read_csv(input_path)
  print(df)
  ax = df.plot.bar()
  plt.show()


def main():
  # action='store_true'
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--input-path', help='The path from which the files should be read', metavar='INPUT_PATH')
  parser.add_argument(
      '--output-path', help='Path at which created files are written to', metavar='OUTPUT_PATH')
  args = parser.parse_args()
  parse(args.input_path, args.output_path)


if __name__ == '__main__':
  main()
