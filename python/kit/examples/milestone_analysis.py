"""Edit milestones, read a calendar and join detached tables."""

from datetime import date, datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory

import holderkit


def main() -> None:
    start = datetime(2026, 10, 5, 9, tzinfo=timezone.utc)
    with TemporaryDirectory(prefix="holder-milestones-") as temporary:
        with holderkit.open(Path(temporary) / "data") as context:
            project = context.create_project("Calendar")
            card = context.create_card(project.project_id, "Review evidence", "Research #review")
            # A milestone has labelled dates: each a day, or an instant with its offset.
            milestone = context.milestones.add(
                project.project_id, card.card_id,
                [{"role": "start", "datetime": start}, {"role": "end", "datetime": start + timedelta(hours=1)}],
                kind="Appointment", description="Review the evidence together",
            )
            updated = context.milestones.update(project.project_id, card.card_id, milestone["milestone_id"], {
                "description": "Bring notes", "dates": [{"role": "deadline", "date": date(2026, 10, 5)}],
            })
            print("Updated:", updated)
            # The calendar for 5 October in London: a day date by its day, an instant by where it falls there.
            print("Calendar:", context.milestones.calendar(
                project.project_id, date(2026, 10, 5), date(2026, 10, 5), time_zone="Europe/London"))
            tables = context.to_dataframes(project.project_id)
        joined = tables["milestones"].merge(tables["cards"], on=["project_id", "card_id"])
        print(joined[["title", "dates", "kind", "description"]].to_string(index=False))


if __name__ == "__main__":
    main()
