#include "frame_vlc.h"

void vlc_frame(Frame& frame, Writer& writer) {
  std::vector<std::array<int, 17>> nc_Y_table;
  nc_Y_table.reserve(frame.mbs.size());
  std::vector<std::array<int, 4>> nc_Cb_table;
  nc_Cb_table.reserve(frame.mbs.size());
  std::vector<std::array<int, 4>> nc_Cr_table;
  nc_Cr_table.reserve(frame.mbs.size());

  // // int mb_no = 0;
  for (auto& mb : frame.mbs) {
  //   // f_logger.log(Level::DEBUG, "mb #" + std::to_string(mb_no++));
    std::array<int, 17> current_Y_table;
    std::array<int, 4> current_Cb_table;
    std::array<int, 4> current_Cr_table;
    nc_Y_table.push_back(current_Y_table);
    nc_Cb_table.push_back(current_Cb_table);
    nc_Cr_table.push_back(current_Cr_table);

    if (mb.is_intra16x16)
      vlc_Y_DC(mb, nc_Y_table, frame);
    for (int i = 0; i != 16; i++)
      vlc_Y(i, mb, nc_Y_table, frame);

    vlc_Cb_DC(mb);
    for (int i = 0; i != 4; i++)
      vlc_Cb_AC(i, mb, nc_Cb_table, frame);

    vlc_Cr_DC(mb);
    for (int i = 0; i != 4; i++)
      vlc_Cr_AC(i, mb, nc_Cr_table, frame);
  }
}

void vlc_Y_DC(MacroBlock& mb, std::vector<std::array<int, 17>>& nc_Y_table, Frame& frame) {
  int nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
  int nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);

  int nC;
  if (nA_index != -1 && nB_index != -1 && frame.mbs.at(nA_index).is_intra16x16 && frame.mbs.at(nB_index).is_intra16x16)
    nC = (nc_Y_table.at(nA_index)[16] + nc_Y_table.at(nB_index)[16] + 1) >> 1;
  else if (nA_index != -1 && frame.mbs.at(nA_index).is_intra16x16)
    nC = nc_Y_table.at(nA_index)[16];
  else if (nB_index != -1 && frame.mbs.at(nB_index).is_intra16x16)
    nC = nc_Y_table.at(nB_index)[16];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero)= cavlc_block4x4(mb.get_Y_DC_block(), nC);

  nc_Y_table.at(mb.mb_index)[16] = non_zero;
}

void vlc_Y(int cur_pos, MacroBlock& mb, std::vector<std::array<int, 17>>& nc_Y_table, Frame& frame) {
  int real_pos = MacroBlock::convert_table[cur_pos];

  int nA_index, nA_pos;
  if (real_pos % 4 == 0) {
    nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
    nA_pos = real_pos + 3;
  } else {
    nA_index = mb.mb_index;
    nA_pos = real_pos - 1;
  }
  nA_pos = MacroBlock::convert_table[nA_pos];

  int nB_index, nB_pos;
  if (0 <= real_pos && real_pos <= 3) {
    nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
    nB_pos = 12 + real_pos;
  } else {
    nB_index = mb.mb_index;
    nB_pos = real_pos - 4;
  }
  nB_pos = MacroBlock::convert_table[nB_pos];

  int nC;
  if (nA_index != -1 && nB_index != -1)
    nC = (nc_Y_table.at(nA_index)[nA_pos] + nc_Y_table.at(nB_index)[nB_pos] + 1) >> 1;
  else if (nA_index != -1)
    nC = nc_Y_table.at(nA_index)[nA_pos];
  else if (nB_index != -1)
    nC = nc_Y_table.at(nB_index)[nB_pos];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  if (mb.is_intra16x16)
    std::tie(bitstream, non_zero)= cavlc_block4x4(mb.get_Y_AC_block(cur_pos), nC);
  else
    std::tie(bitstream, non_zero)= cavlc_block4x4(mb.get_Y_4x4_block(cur_pos), nC);

  nc_Y_table.at(mb.mb_index)[cur_pos] = non_zero;
}

void vlc_Cb_DC(MacroBlock& mb) {
  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero)= cavlc_block2x2(mb.get_Cb_DC_block(), -1);
}

void vlc_Cr_DC(MacroBlock& mb) {
  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero)= cavlc_block2x2(mb.get_Cr_DC_block(), -1);
}

void vlc_Cb_AC(int cur_pos, MacroBlock& mb, std::vector<std::array<int, 4>>& nc_Cb_table, Frame& frame) {
  int nA_index, nA_pos;
  if (cur_pos % 2 == 0) {
    nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
    nA_pos = cur_pos + 1;
  } else {
    nA_index = mb.mb_index;
    nA_pos = cur_pos - 1;
  }

  int nB_index, nB_pos;
  if (0 <= cur_pos && cur_pos <= 3) {
    nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
    nB_pos = cur_pos + 2;
  } else {
    nB_index = mb.mb_index;
    nB_pos = cur_pos - 2;
  }

  int nC;
  if (nA_index != -1 && nB_index != -1)
    nC = (nc_Cb_table.at(nA_index)[nA_pos] + nc_Cb_table.at(nB_index)[nB_pos] + 1) >> 1;
  else if (nA_index != -1)
    nC = nc_Cb_table.at(nA_index)[nA_pos];
  else if (nB_index != -1)
    nC = nc_Cb_table.at(nB_index)[nB_pos];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero)= cavlc_block4x4(mb.get_Cb_AC_block(cur_pos), nC);

  nc_Cb_table.at(mb.mb_index)[cur_pos] = non_zero;
}

void vlc_Cr_AC(int cur_pos, MacroBlock& mb, std::vector<std::array<int, 4>>& nc_Cr_table, Frame& frame) {
  int nA_index, nA_pos;
  if (cur_pos % 2 == 0) {
    nA_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_L);
    nA_pos = cur_pos + 1;
  } else {
    nA_index = mb.mb_index;
    nA_pos = cur_pos - 1;
  }

  int nB_index, nB_pos;
  if (0 <= cur_pos && cur_pos <= 3) {
    nB_index = frame.get_neighbor_index(mb.mb_index, MB_NEIGHBOR_U);
    nB_pos = cur_pos + 2;
  } else {
    nB_index = mb.mb_index;
    nB_pos = cur_pos - 2;
  }

  int nC;
  if (nA_index != -1 && nB_index != -1)
    nC = (nc_Cr_table.at(nA_index)[nA_pos] + nc_Cr_table.at(nB_index)[nB_pos] + 1) >> 1;
  else if (nA_index != -1)
    nC = nc_Cr_table.at(nA_index)[nA_pos];
  else if (nB_index != -1)
    nC = nc_Cr_table.at(nB_index)[nB_pos];
  else
    nC = 0;

  Bitstream bitstream;
  int non_zero;
  std::tie(bitstream, non_zero)= cavlc_block4x4(mb.get_Cr_AC_block(cur_pos), nC);

  nc_Cr_table.at(mb.mb_index)[cur_pos] = non_zero;
}
