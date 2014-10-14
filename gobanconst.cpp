/***************************************************************************************
* Copyright (c) 2014, Antonio Garro.                                                   *
* All rights reserved                                                                  *
*                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are *
* permitted provided that the following conditions are met:                            *
*                                                                                      *
* 1. Redistributions of source code must retain the above copyright notice, this list  *
* of conditions and the following disclaimer.                                          *
*                                                                                      *
* 2. Redistributions in binary form must reproduce the above copyright notice, this    *
* list of conditions and the following disclaimer in the documentation and/or other    *
* materials provided with the distribution.                                            *
*                                                                                      *
* 3. Neither the name of the copyright holder nor the names of its contributors may be *
* used to endorse or promote products derived from this software without specific      *
* prior written permission.                                                            *
*                                                                                      * 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"          *
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE            *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE           *
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE            *
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL    *
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR           *
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER           *
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR     *
* TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF        *
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                    *
***************************************************************************************/
#include <iostream>
#include "goban.h"

#ifdef ZOBRIST
unsigned long long Goban::get_zobrist() const
{
	return zobrist.get_key();
}

unsigned long long Goban::get_zobrist(int move)const
{
	unsigned long long zob_backup = get_zobrist();
	unsigned long long new_zobrist = set_zobrist(move);
	zobrist.set_key(zob_backup);
	return new_zobrist;
}

unsigned long long Goban::set_zobrist(int move) const
{
	//TODO: ko zobrist;
	if(move){
		zobrist.update(move, side);
		
		GroupSet<4> neighbours;
		int nneigh = neighbour_groups(move, neighbours);
		
		for(int i = 0; i < nneigh; i++){
			Group *current_neigh = neighbours[i];
			if(current_neigh->get_color() != side){
				if(current_neigh->has_one_liberty()){
					for(Group::StoneIterator st(current_neigh); st; ++st){
						zobrist.update(*st, current_neigh->get_color());
					}
				}
			}
		}
	}
	zobrist.toggle_side();
	return get_zobrist();
}
#endif
int Goban::point_liberties(int point, PList &liberties) const
{	// liberties must be NULL or of size > 4.
	for (int i = 0; adjacent[point][i]; i++){
		if(points[adjacent[point][i]] == 0){
			liberties.add(adjacent[point][i]);
		}
	}
	return liberties.length();
}

int Goban::point_liberties(int point, int TODO) const
{	// liberties[] must be NULL or of size > 4.
	int nlibs = 0;
	for (int i = 0; adjacent[point][i]; i++){
		if(points[adjacent[point][i]] == 0){
			nlibs++;
		}
	}
	return nlibs;
}

int Goban::neighbour_groups(int point, GroupSet<4> &neighbours) const
{
	for (int i = 0; adjacent[point][i]; i++){
		neighbours.add(points[adjacent[point][i]]);
	}
	return neighbours.length();	
}

int Goban::neighbour_groups(int point, bool color, int max_liberties, GroupSet<MAXSIZE2/3> *neighbours) const
{
	int nneigh = 0;
	for (int i = 0; adjacent[point][i]; i++){
		Group *current_group = points[adjacent[point][i]];

		if(current_group && current_group->get_color() == color && current_group->get_nliberties() <= max_liberties){
			if(neighbours) neighbours->add(current_group);
			nneigh++;
		}
	}
	return nneigh;	
}

int Goban::neighbour_groups(const Group *group, bool color, int max_liberties, GroupSet<MAXSIZE2/3> &neighbours) const
{
	for(Group::StoneIterator st(group); st; ++st){
		neighbour_groups(*st, color, max_liberties, &neighbours);
	}
	return neighbours.length();
}

int Goban::neighbours_in_atari(int point, bool color, const GroupSet<4> &neighbours) const
{
	int natari = 0;
	for (int i = 0; i < neighbours.length(); i++){
		if(neighbours[i]->get_color() !=color && neighbours[i]->has_one_liberty()){
			natari++;
		}
	}
	return natari;
}

bool Goban::is_false_eye(int point, bool color, int consider_occupied) const
{
	if(points[point] != 0) return false;
	for (int i = 0; adjacent[point][i]; i++){
		if(adjacent[point][i] == consider_occupied) continue;
		if(points[adjacent[point][i]] == 0){
			return false;
		}
		if(points[adjacent[point][i]]->get_color() != color){
			return false;
		}
	}
	return true;
}

bool Goban::is_eye(int point, bool color, int consider_occupied) const
{
	int i, ncontrolled = 0;
	if(!is_false_eye(point, color, consider_occupied)) return false;
	for (i = 0; diagonals[point][i]; i++){
			if(points[diagonals[point][i]]){
				if(points[diagonals[point][i]]->get_color() == color){
					ncontrolled++;
				}
			}
			else{
				if(is_false_eye(diagonals[point][i], color, consider_occupied)){
					ncontrolled++;
				}
			}
	}
	if(i == 4) {
		if (ncontrolled > 2) return true;
	}
	else{
		if (ncontrolled == i) return true;
	}
	return false;
}

int Goban::neighbours_size(int point, bool color) const
{
	int nstones = 0;
	GroupSet<4> neighbours;
	int nneigh = neighbour_groups(point, neighbours);
	for(int i = 0; i < nneigh; i++){
		const Group *curr_neigh = neighbours[i];
		if(curr_neigh->get_color() == color){
			nstones += curr_neigh->get_nstones();
		}
	}
	return nstones;
}

bool Goban::is_legal(int point, bool color) const
{	//asumes an empty point in the range [1, size2]
	if(point == ko_point) return false;
#ifdef ZOBRIST
	if(zobrist.check_history(get_zobrist(point))) return false;
#endif
	int nlibs = point_liberties(point, 0);
	if(nlibs > 0) return true;

	GroupSet<4> neighbours;
	int nneigh = neighbour_groups(point, neighbours);
	for(int i = 0; i < nneigh; i++){
		if(neighbours[i]->get_color() == color && !neighbours[i]->has_one_liberty()){
			return true;
		}
	}
	int natari = neighbours_in_atari(point, color, neighbours);
	return natari > 0;
}

int Goban::legal_moves(int moves[]) const
{
	int nlegal = 0, point;
	for(int i = 0; i < empty_points.length(); i++){
		point = empty_points[i];
		if (is_legal(point, side)){
			moves[nlegal++] = point;
		}
	}
	moves[nlegal++] = PASS;
	return nlegal;
}

int Goban::get_color(int point) const	//Carefull, this returns 1,-1,0!!!
{
	if(points[point]){
		if(points[point]->get_color()) return -1;
		else return 1;
	}
	return 0;
}

float Goban::chinese_count() const
{
	int black_score = 0, white_score = 0, eyes_result = 0;
	for(int i = 1; i <= size2; i++){
		if(points[i] != 0){
			if(points[i]->get_color()) white_score++;
			else black_score++;
		}
		else{
			if(is_false_eye(i, 0)) eyes_result++;
			else if(is_false_eye(i, 1)) eyes_result--;
		}
	}
	return eyes_result + black_score - white_score - komi;
}

 void Goban::score_area(int point_list[]) const
{
	for(int i = 1; i <= size2; i++){
		if(points[i] != 0){
			if(points[i]->get_color()) point_list[i] -= 1;
			else point_list[i] += 1;
		}
		else{
			if(is_false_eye(i, 0)) point_list[i] += 1;
			else if(is_false_eye(i, 1)) point_list[i] -= 1;
		}
	}
}
int Goban::mercy() const
{
	for(int s = 0; s < 2; s++){
		if(stones_on_board[s] - stones_on_board[1-s] > size2/3){
			return s; 
		}
	}
	return -1;
}

void Goban::print_goban() const
{
	std::cerr << "   ";
	for(int i = 0; i<size; i++) std::cerr << "--";
	std::cerr << "\n";
	for (int y = size - 1; y > -1; y--){
		std::cerr << "  |";
		for (int x = 1; x < size+1; x++){
			if (points[size*y + x]){
				if (points[size*y + x]->get_color()) std::cerr << "# ";
				else std::cerr << "o ";
			}
			else std::cerr << ". ";
		}
		std::cerr << "|" << y+1 << "\n";
	}
	std::cerr << "   ";
	for(int i = 0; i<size; i++) std::cerr << "--";
	std::cerr << "\n  ";
	for(int i = 0; i<size; i++) std::cerr << " " << COORDINATES[i];
	std::cerr << "\nMoves: " << game_history.length() << " Side: " << side << " Komi: " << komi << " empty: " << empty_points.length();
#ifdef ZOBRIST
	std::cerr << "\nZobrist: " << zobrist.get_key();
#endif
	int lat1 = (last_atari[0] ? last_atari[0]->get_stone(0): 0);
	int lat2 = (last_atari[1] ? last_atari[1]->get_stone(0) : 0);
	std::cerr << " last atari: black " << lat1<< " white " << lat2 << "\n";
}
