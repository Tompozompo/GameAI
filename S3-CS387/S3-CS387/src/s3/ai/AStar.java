package s3.ai;


import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import gatech.mmpm.Entity;
import s3.base.S3;
import s3.entities.S3PhysicalEntity;
import s3.util.Pair;


public class AStar {

	private Node start;
	private Node goal;
	private S3 game;
	private S3PhysicalEntity entity;
	private List<Pair<Integer, Integer>> directions;

	public static int pathDistance(double start_x, double start_y, double goal_x, double goal_y,
			S3PhysicalEntity i_entity, S3 the_game) {
		AStar a = new AStar(start_x,start_y,goal_x,goal_y,i_entity,the_game);
		List<Pair<Double, Double>> path = a.computePath();
		if (path!=null) return path.size();
		return -1;
	}

	public AStar(double start_x, double start_y, double goal_x, double goal_y,
			S3PhysicalEntity i_entity, S3 the_game) {
		start = new Node(start_x, start_y);
		goal = new Node(goal_x, goal_y);
		game = the_game;
		entity = i_entity;
		directions = new ArrayList<Pair<Integer, Integer>>();
		directions.add(new Pair<Integer, Integer>(-1,0));
		directions.add(new Pair<Integer, Integer>(1,0));
		directions.add(new Pair<Integer, Integer>(0,-1));
		directions.add(new Pair<Integer, Integer>(0,1));
	}

	public List<Pair<Double, Double>> computePath() {
		start.g = 0;
		start.h = heuristic(start);
		ArrayList<Node> open = new ArrayList<Node>();
		open.add(start);
		HashSet<Node> closed = new HashSet<Node>();
		while (!open.isEmpty()) {
			Node n = removeLowestF(open);
			if (isGoal(n)) {
				return getPath(n);
			}
			closed.add(n);
			for (Pair<Integer, Integer> d : directions) {
				double x = n.x + d.m_a;
				double y = n.y + d.m_b;
				S3PhysicalEntity e = (S3PhysicalEntity) entity.clone();
				e.setX((int)x);
				e.setY((int)y);
				if (x < 0 || y < 0 || x >= game.getMap().getWidth() || y >= game.getMap().getHeight()) {
					continue;
				}
				if (game.anyLevelCollision(e) == null) {
					Node m = new Node(x, y);
					if (!open.contains(m) && !closed.contains(m)) {
						m.parent = n;
						m.g = n.g + 1;
						m.h = heuristic(m);
						open.add(m);
					}
				}
			}
		}
		return null;
	}

	private List<Pair<Double, Double>> getPath(Node n) {
		List<Pair<Double, Double>> path = new ArrayList<Pair<Double, Double>>();
		while(n != start) {
			Pair<Double, Double> e = new Pair<Double, Double>(n.x, n.y);
			path.add(0, e);
			n = n.parent;
		}
		return path;
	}

	private boolean isGoal(Node n) {
		return (n.x == goal.x && n.y == goal.y);
	}

	private Node removeLowestF(ArrayList<Node> open) {
		Node min = null;
		double min_f = 0;
		for(Node n : open) {
			double f = n.g + n.h;
			if(min == null || f < min_f) {
				min = n;
				min_f = f;
			}
		}
		if(min != null) {
			open.remove(min);
		}
		return min;
	}

	private double heuristic(Node n) {
		return manhattanDist(n);
	}

	private double manhattanDist(Node n) {
		return Math.abs(goal.x-n.x) + Math.abs(goal.y-n.y);
	}

	private class Node {
		Node parent;
		double x;
		double y;
		double g;
		double h;

		private Node(double x, double y) {
			this.x = x;
			this.y = y;
		}

		@Override
		public int hashCode() {
			final int prime1 = 31;
			final int prime2 = 17;
			int result = 0;
			result += prime1 * x;
			result += prime2 * y;
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			Node other = (Node)obj;
			if (x != other.x)
				return false;
			if (y != other.y)
				return false;
			return true;
		}
	}
}

